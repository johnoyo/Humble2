#include "PoolArena.h"

namespace HBL2
{
    PoolArena::~PoolArena()
    {
        if (!m_Destructed)
        {
            Destroy();
        }
    }

    void PoolArena::Initialize(MainArena* global, size_t totalBytes, size_t blockSize, PoolReservation* reservation, size_t blockAlign)
    {
        HBL2_CORE_ASSERT(global, "PoolArena::Initialize: global is null");
        HBL2_CORE_ASSERT(reservation, "PoolArena::Initialize: reservation is null");
        HBL2_CORE_ASSERT(totalBytes != 0, "PoolArena::Initialize: totalBytes == 0");
        HBL2_CORE_ASSERT(blockSize != 0, "PoolArena::Initialize: blockSize == 0");

        // Enforce power-of-two alignment
        if ((blockAlign & (blockAlign - 1)) != 0)
        {
            HBL2_CORE_ASSERT(false, "PoolArena::Initialize: blockAlign must be power of two");
        }

        m_GlobalArena = global;
        m_Reservation = reservation;
        m_Reservation->RefCount.fetch_add(1, std::memory_order_relaxed);

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
            HBL2_CORE_ERROR("PoolArena ERROR: totalBytes {} too small for blockSize {}\n", totalBytes, m_BlockSize);
            throw std::bad_alloc();
        }

        // Allocate next[] in META and build free list as before...
        void* nextMem = m_GlobalArena->AllocMeta(sizeof(std::atomic<uint32_t>) * m_BlockCount, alignof(std::atomic<uint32_t>));
        if (!nextMem)
        {
            HBL2_CORE_ERROR("PoolArena ERROR: meta exhausted allocating next array ({} entries)\n", m_BlockCount);
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

    void PoolArena::Destroy()
    {
        if (m_Reservation)
        {
            m_Reservation->RefCount.fetch_sub(1, std::memory_order_relaxed);
        }

        if (m_GlobalArena)
        {
            m_GlobalArena->TryRelease(m_Reservation);
        }

        m_Destructed = true;
    }

    void* PoolArena::Alloc(size_t size, size_t alignment)
    {
        if (size == 0)
        {
            return nullptr;
        }

        // Fixed-block allocator constraints
        if (size > m_BlockSize)
        {
            HBL2_CORE_ERROR("PoolArena ERROR: request size {} > block size {}", size, m_BlockSize);
            throw std::bad_alloc();
        }
        if (alignment > m_BlockAlign)
        {
            // Because blocks are aligned to m_BlockAlign; larger alignment would not be guaranteed.
            HBL2_CORE_ERROR("PoolArena ERROR: request alignment {} > pool block alignment {}", alignment, m_BlockAlign);
            throw std::bad_alloc();
        }

        const uint32_t idx = PopIndex();
        if (idx == InvalidIndex)
        {
            HBL2_CORE_ERROR("PoolArena ERROR: exhausted (blocks={}, blockSize={})", m_BlockCount, m_BlockSize);
            throw std::bad_alloc();
        }

#ifdef ARENA_DEBUG
        UpdateStats(+1);
#endif
        return static_cast<void*>(m_Data + (static_cast<size_t>(idx) * m_BlockSize));
    }

    void PoolArena::Free(void* p)
    {
        if (!p)
        {
            return;
        }

        uint8_t* up = static_cast<uint8_t*>(p);
        if (up < m_Data || up >= (m_Data + m_TotalBytes))
        {
            // Strict but non-fatal: ignore or assert; your choice.
            HBL2_CORE_ASSERT(false, "PoolArena::Free pointer out of range");
            return;
        }

        const size_t off = static_cast<size_t>(up - m_Data);
        if ((off % m_BlockSize) != 0)
        {
            HBL2_CORE_ASSERT(false, "PoolArena::Free pointer not block-aligned");
            return;
        }

        const uint32_t idx = static_cast<uint32_t>(off / m_BlockSize);
        if (idx >= m_BlockCount)
        {
            HBL2_CORE_ASSERT(false, "PoolArena::Free index out of range");
            return;
        }

        PushIndex(idx);

#ifdef ARENA_DEBUG
        UpdateStats(-1);
#endif
    }

    uint32_t PoolArena::PopIndex()
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
    void PoolArena::PushIndex(uint32_t idx)
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
    void PoolArena::UpdateStats(int32_t delta)
    {
#ifdef ARENA_DEBUG

        int32_t cur = m_InUse.fetch_add(delta, std::memory_order_relaxed) + delta;
        int32_t hw = m_HighWater.load(std::memory_order_relaxed);
        while (cur > hw && !m_HighWater.compare_exchange_weak(hw, cur, std::memory_order_relaxed)) {}
#endif
    }
}
