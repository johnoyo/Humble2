#pragma once

#include "MainArena.h"

namespace HBL2
{
    /**
     * @brief PoolArena: fixed-size block allocator with lock-free free list (MPMC).
     *
     * - Thread-safe, lock-free allocations/frees (index-based tagged stack).
     * - Strict: no fallback malloc; exhaustion throws std::bad_alloc.
     * - Memory comes from MainArena DATA (optionally from a PoolReservation).
     * - Metadata (free-list "next" array) is placement-new'd in MainArena META.
     *
     * Intended use:
     *   PoolArena pool(&global, /*bytes* / 4*1024*1024, /*blockSize* / 256, reservation);
     *   void* p = pool.Alloc(128, alignof(SomeType));
     *   pool.Free(p);
     */
    class PoolArena
    {
    public:
        PoolArena() = default;

        PoolArena(MainArena* global, size_t totalBytes, size_t blockSize, PoolReservation* reservation = nullptr)
        {
            Initialize(global, totalBytes, blockSize, reservation);
        }

        ~PoolArena() = default; // No chunk structs to return; underlying DATA is owned by MainArena.

        /**
         * @brief Initialize the pool.
         *
         * @param global       MainArena backing store.
         * @param totalBytes   Total bytes to carve for pool storage.
         * @param blockSize    Fixed block size returned by Alloc (aligned up internally).
         * @param reservation  Optional reservation to carve from.
         */
        void Initialize(MainArena* global, size_t totalBytes, size_t blockSize, PoolReservation* reservation = nullptr, size_t blockAlign = 16)
        {
            if (!global) throw std::invalid_argument("PoolArena::Initialize: global is null");
            if (totalBytes == 0) throw std::invalid_argument("PoolArena::Initialize: totalBytes == 0");
            if (blockSize == 0) throw std::invalid_argument("PoolArena::Initialize: blockSize == 0");

            // Enforce power-of-two alignment
            if ((blockAlign & (blockAlign - 1)) != 0)
            {
                throw std::invalid_argument("PoolArena::Initialize: blockAlign must be power of two");
            }

            m_GlobalArena = global;
            m_Reservation = reservation;

            // Guarantee at least 16B alignment by default; user can pass 32/64/etc.
            m_BlockAlign = blockAlign;
            m_BlockSize = AlignUp(blockSize, m_BlockAlign);

            // Carve a bit extra so we can align the base ourselves even if CarveData is only 8-aligned
            const size_t extra = m_BlockAlign - 1;
            m_RawData = m_GlobalArena->CarveData(totalBytes + extra, m_Reservation);

            // Align the usable base pointer
            uintptr_t raw = reinterpret_cast<uintptr_t>(m_RawData);
            uintptr_t aligned = AlignUp(raw, m_BlockAlign);
            m_Data = reinterpret_cast<uint8_t*>(aligned);

            // Effective usable bytes from aligned base (still within carved region)
            const size_t shift = static_cast<size_t>(aligned - raw);
            m_EffectiveBytes = totalBytes; // we promised totalBytes usable after alignment
            // (This is safe because we carved totalBytes + extra, and shift <= extra)

            m_TotalBytes = m_EffectiveBytes;

            // Compute block count from effective bytes
            m_BlockCount = static_cast<uint32_t>(m_TotalBytes / m_BlockSize);
            if (m_BlockCount == 0)
            {
                ADBG("PoolArena ERROR: totalBytes (%zu) too small for blockSize (%zu)\n", totalBytes, m_BlockSize);
                throw std::bad_alloc();
            }

            // Allocate next[] in META and build free list as before...
            void* nextMem = m_GlobalArena->AllocMeta(sizeof(std::atomic<uint32_t>) * m_BlockCount, alignof(std::atomic<uint32_t>));
            if (!nextMem)
            {
                ADBG("PoolArena ERROR: meta exhausted allocating next array (%u entries)\n", m_BlockCount);
                throw std::bad_alloc();
            }

            m_Next = static_cast<std::atomic<uint32_t>*>(nextMem);
            for (uint32_t i = 0; i < m_BlockCount; ++i)
            {
                ::new (static_cast<void*>(m_Next + i)) std::atomic<uint32_t>(InvalidIndex);
            }

            for (uint32_t i = 0; i + 1 < m_BlockCount; ++i)
            {
                m_Next[i].store(i + 1, std::memory_order_relaxed);
            }
            m_Next[m_BlockCount - 1].store(InvalidIndex, std::memory_order_relaxed);

            m_HeadTagged.store(PackTagged(0u, 0u), std::memory_order_release);

            #ifdef ARENA_DEBUG
            m_InUse.store(0);
            m_HighWater.store(0);
            #endif
        }

        /**
         * @brief Allocate up to m_BlockSize bytes. Lock-free. Thread-safe.
         *
         * @throws std::bad_alloc if exhausted or request too large/alignment too strict.
         */
        void* Alloc(size_t size, size_t alignment = alignof(std::max_align_t))
        {
            if (size == 0) return nullptr;

            // Fixed-block allocator constraints
            if (size > m_BlockSize)
            {
                ADBG("PoolArena ERROR: request size %zu > block size %zu\n", size, m_BlockSize);
                throw std::bad_alloc();
            }
            if (alignment > m_BlockAlign)
            {
                // Because blocks are aligned to m_BlockAlign; larger alignment would not be guaranteed.
                ADBG("PoolArena ERROR: request alignment %zu > pool block alignment %zu\n", alignment, m_BlockAlign);
                throw std::bad_alloc();
            }

            const uint32_t idx = PopIndex();
            if (idx == InvalidIndex)
            {
                ADBG("PoolArena ERROR: exhausted (blocks=%u, blockSize=%zu)\n", m_BlockCount, m_BlockSize);
                throw std::bad_alloc();
            }

            #ifdef ARENA_DEBUG
            UpdateStats(+1);
            #endif
            return static_cast<void*>(m_Data + (static_cast<size_t>(idx) * m_BlockSize));
        }

        /**
         * @brief Free a previously allocated block. Lock-free. Thread-safe.
         *
         * Undefined behavior if the pointer didn't come from this PoolArena.
         */
        void Free(void* p)
        {
            if (!p) return;

            uint8_t* up = static_cast<uint8_t*>(p);
            if (up < m_Data || up >= (m_Data + m_TotalBytes))
            {
                // Strict but non-fatal: ignore or assert; your choice.
                assert(false && "PoolArena::Free pointer out of range");
                return;
            }

            const size_t off = static_cast<size_t>(up - m_Data);
            if ((off % m_BlockSize) != 0)
            {
                assert(false && "PoolArena::Free pointer not block-aligned");
                return;
            }

            const uint32_t idx = static_cast<uint32_t>(off / m_BlockSize);
            if (idx >= m_BlockCount)
            {
                assert(false && "PoolArena::Free index out of range");
                return;
            }

            PushIndex(idx);

            #ifdef ARENA_DEBUG
            UpdateStats(-1);
            #endif
        }

        template<typename T, typename... Args>
        T* AllocConstruct(Args&&... args)
        {
            void* mem = Alloc(sizeof(T), alignof(T));
            return ::new (mem) T(std::forward<Args>(args)...);
        }

        template<typename T>
        void Destruct(T* object)
        {
            try { object->~T(); }
            catch (...) {}
        }

        size_t BlockSize()   const { return m_BlockSize; }
        uint32_t BlockCount() const { return m_BlockCount; }
        size_t CapacityBytes() const { return static_cast<size_t>(m_BlockCount) * m_BlockSize; }

        #ifdef ARENA_DEBUG
        int32_t InUseBlocks() const { return m_InUse.load(); }
        int32_t HighWaterBlocks() const { return m_HighWater.load(); }
        uint8_t* DebugBase() const { return m_Data; }
        size_t DebugTotalBytes() const { return m_TotalBytes; }
        size_t DebugBlockSize() const { return m_BlockSize; }
        uint32_t DebugBlockCount() const { return m_BlockCount; }
        #endif

    private:
        static constexpr uint32_t InvalidIndex = 0xFFFFFFFFu;

        // Tagged head: low 32 bits = index, high 32 bits = tag (ABA mitigation for index stack).
        static inline uint64_t PackTagged(uint32_t index, uint32_t tag)
        {
            return (static_cast<uint64_t>(tag) << 32) | static_cast<uint64_t>(index);
        }

        static inline uint32_t UnpackIndex(uint64_t tagged)
        {
            return static_cast<uint32_t>(tagged & 0xFFFFFFFFull);
        }

        static inline uint32_t UnpackTag(uint64_t tagged)
        {
            return static_cast<uint32_t>((tagged >> 32) & 0xFFFFFFFFull);
        }

        uint32_t PopIndex()
        {
            // MPMC lock-free pop (Treiber with tag)
            for (;;)
            {
                uint64_t head = m_HeadTagged.load(std::memory_order_acquire);
                const uint32_t idx = UnpackIndex(head);
                const uint32_t tag = UnpackTag(head);

                if (idx == InvalidIndex)
                {
                    return InvalidIndex;
                }

                const uint32_t next = m_Next[idx].load(std::memory_order_acquire);
                const uint64_t desired = PackTagged(next, tag + 1);

                if (m_HeadTagged.compare_exchange_weak(head, desired, std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    // Optional: poison next[idx] to catch bugs
                    // m_Next[idx].store(InvalidIndex, std::memory_order_relaxed);
                    return idx;
                }
            }
        }

        void PushIndex(uint32_t idx)
        {
            // MPMC lock-free push (Treiber with tag)
            for (;;)
            {
                uint64_t head = m_HeadTagged.load(std::memory_order_acquire);
                const uint32_t cur = UnpackIndex(head);
                const uint32_t tag = UnpackTag(head);

                m_Next[idx].store(cur, std::memory_order_release);
                const uint64_t desired = PackTagged(idx, tag + 1);

                if (m_HeadTagged.compare_exchange_weak(head, desired, std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    return;
                }
            }
        }

        #ifdef ARENA_DEBUG
        void UpdateStats(int32_t delta)
        {
            int32_t cur = m_InUse.fetch_add(delta, std::memory_order_relaxed) + delta;
            int32_t hw = m_HighWater.load(std::memory_order_relaxed);
            while (cur > hw && !m_HighWater.compare_exchange_weak(hw, cur, std::memory_order_relaxed)) {}
        }
        #endif

    private:
        MainArena* m_GlobalArena = nullptr;
        PoolReservation* m_Reservation = nullptr;

        uint8_t* m_Data = nullptr;
        size_t m_TotalBytes = 0;

        size_t m_BlockSize = 0;
        size_t m_BlockAlign = 0;
        uint32_t m_BlockCount = 0;

        uint8_t* m_RawData = nullptr;   // raw pointer returned by CarveData
        size_t m_EffectiveBytes = 0;  // bytes usable from m_Data (aligned base)

        // Free list storage (indices)
        std::atomic<uint32_t>* m_Next = nullptr;

        // Tagged head: index+tag to reduce ABA on head CAS
        std::atomic<uint64_t> m_HeadTagged{ PackTagged(InvalidIndex, 0u) };

        #ifdef ARENA_DEBUG
        std::atomic<int32_t> m_InUse{ 0 };
        std::atomic<int32_t> m_HighWater{ 0 };
        #endif
    };
}