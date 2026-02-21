#pragma once

#include "MainArena.h"

namespace HBL2
{
    /**
     * @brief PoolArena: fixed-size block allocator with lock-free free list (MPMC).
     *
     * - Thread-safe, lock-free allocations/frees (index-based tagged stack).
     * - Memory comes from MainArena DATA from a PoolReservation.
     * - Metadata (free-list "next" array) is placement-new'd in MainArena META.
     */
    class HBL2_API PoolArena
    {
    public:
        PoolArena() = default;

        ~PoolArena();

        /**
         * @brief Initialize the pool.
         *
         * @param global       MainArena backing store.
         * @param totalBytes   Total bytes to carve for pool storage.
         * @param blockSize    Fixed block size returned by Alloc (aligned up internally).
         * @param reservation  Optional reservation to carve from.
         */
        void Initialize(MainArena* global, size_t totalBytes, size_t blockSize, PoolReservation* reservation, size_t blockAlign = 16);

        /**
         * @brief Destruct the pool.
         */
        void Destroy();

        /**
         * @brief Allocate up to m_BlockSize bytes. Lock-free. Thread-safe.
         *
         * @throws std::bad_alloc if exhausted or request too large/alignment too strict.
         */
        void* Alloc(size_t size, size_t alignment = alignof(std::max_align_t));

        /**
         * @brief Free a previously allocated block. Lock-free. Thread-safe.
         *
         * Undefined behavior if the pointer didn't come from this PoolArena.
         */
        void Free(void* p);

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

        inline size_t BlockSize()   const { return m_BlockSize; }
        inline uint32_t BlockCount() const { return m_BlockCount; }
        inline size_t CapacityBytes() const { return static_cast<size_t>(m_BlockCount) * m_BlockSize; }

#ifdef ARENA_DEBUG
        inline int32_t InUseBlocks() const { return m_InUse.load(); }
        inline int32_t HighWaterBlocks() const { return m_HighWater.load(); }
        inline uint8_t* DebugBase() const { return m_Data; }
        inline size_t DebugTotalBytes() const { return m_TotalBytes; }
        inline size_t DebugBlockSize() const { return m_BlockSize; }
        inline uint32_t DebugBlockCount() const { return m_BlockCount; }
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

        uint32_t PopIndex();

        void PushIndex(uint32_t idx);

        void UpdateStats(int32_t delta);

    private:
        MainArena* m_GlobalArena = nullptr;
        PoolReservation* m_Reservation = nullptr;
        bool m_Destructed = false;

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