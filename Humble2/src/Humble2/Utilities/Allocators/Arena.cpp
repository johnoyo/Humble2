#include "Arena.h"

namespace HBL2
{
	Arena::~Arena()
    {
        if (!m_Destructed)
        {
            Destroy();
        }
    }

    void Arena::Initialize(MainArena* global, size_t bytes, PoolReservation* reservation)
    {
        m_GlobalArena = global;

        m_Bytes = bytes;
        m_Reservation = reservation;
        m_Reservation->RefCount.fetch_add(1, std::memory_order_relaxed);
        m_Chunk = nullptr;
        m_Destructed = false;

        m_Chunk = m_GlobalArena->AllocateChunkStruct(bytes, m_Reservation);

        m_Used.store(0);
        m_HighWater.store(0);
    }

    void Arena::Destroy()
    {
        // Return chunk struct to per-reservation or global free lists
        m_GlobalArena->FreeChunkStruct(m_Chunk);
        m_Chunk = nullptr;

        m_Bytes = 0;
        m_Used.store(0);
        m_HighWater.store(0);

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

    void* Arena::Alloc(size_t size, size_t alignment)
    {
        if (size == 0)
        {
            return nullptr;
        }

        ArenaChunk* ch = m_Chunk;

        if (!m_Chunk)
        {
            return nullptr;
        }

        if (ch && ch->HasSpace(size, alignment))
        {
            uintptr_t base = reinterpret_cast<uintptr_t>(ch->Data);
            uintptr_t cur = base + ch->Used;
            uintptr_t aligned = AlignUp(cur, alignment);
            size_t offset = static_cast<size_t>(aligned - base);
            ch->Used = offset + size;
#ifdef ARENA_DEBUG
            UpdateStats(size);
#endif
            return static_cast<void*>(ch->Data + offset);
        }

        HBL2_CORE_FATAL("Arena out of memory!");

        return nullptr;
    }

    Arena::Marker Arena::Mark() const
    {
        Marker m{};

        if (m_Chunk != nullptr)
        {
            m.UsedInLast = m_Chunk->Used;
        };

        return m;
    }

    void Arena::Restore(const Arena::Marker& m)
    {
        if (m_Chunk != nullptr)
        {
            m_Chunk->Used = m.UsedInLast;
        }

#ifdef ARENA_DEBUG
        RecalcStats();
#endif
    }

    void Arena::Reset()
    {
        if (m_Chunk != nullptr)
        {
            m_Chunk->Used = 0;
        }

#ifdef ARENA_DEBUG
        RecalcStats();
#endif
    }

    void Arena::UpdateStats(size_t delta)
    {
        size_t prev = m_Used.fetch_add(delta);
        size_t cur = prev + delta;
        size_t oldhw = m_HighWater.load();
        while (cur > oldhw && !m_HighWater.compare_exchange_weak(oldhw, cur)) {}
    }

    void Arena::RecalcStats()
    {
        m_UsedBeforeReset.store(m_Used.load());

        size_t total = 0;

        if (m_Chunk != nullptr)
        {
            total += m_Chunk->Used;
        }

        m_Used.store(total);
        size_t hw = m_HighWater.load();
        if (total > hw)
        {
            m_HighWater.store(total);
        }
    }
}
