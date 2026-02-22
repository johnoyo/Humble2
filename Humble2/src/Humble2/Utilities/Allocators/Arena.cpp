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
        m_Current = nullptr;
        m_NextChunkSize = bytes;
        m_Destructed = false;

        AcquireChunk(m_NextChunkSize);

        m_Used.store(0);
        m_HighWater.store(0);
    }

    void Arena::Destroy()
    {
        // Return chunk structs to per-reservation or global free lists
        for (ArenaChunk* ch : m_Chunks)
        {
            m_GlobalArena->FreeChunkStruct(ch);
        }

        m_Chunks.clear();
        m_Current = nullptr;

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

        ArenaChunk* ch = m_Current;

        if (!m_Current)
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

        // Request new chunk.
        size_t requestSize = std::max<size_t>(size + alignment, m_NextChunkSize);
        if (m_NextChunkSize < requestSize)
        {
            m_NextChunkSize = requestSize;
        }
        else
        {
            m_NextChunkSize = std::min(m_NextChunkSize * 2, requestSize);
        }

        if (!AcquireChunk(requestSize))
        {
            return nullptr;
        }

        if (!m_Current)
        {
            return nullptr;
        }

        ch = m_Current;
        uintptr_t base = reinterpret_cast<uintptr_t>(ch->Data);
        uintptr_t cur = base + ch->Used;
        uintptr_t aligned = AlignUp(cur, alignment);
        size_t offset = static_cast<size_t>(aligned - base);
        assert(offset + size <= ch->Capacity);
        ch->Used = offset + size;
#ifdef ARENA_DEBUG
        UpdateStats(size);
#endif
        return static_cast<void*>(ch->Data + offset);
    }

    Arena::Marker Arena::Mark() const
    {
        Marker m{};
        m.ChunkCount = m_Chunks.size();
        m.UsedInLast = m_Chunks.empty() ? 0 : m_Chunks.back()->Used;
        return m;
    }
    void Arena::Restore(const Arena::Marker& m)
    {
        while (m_Chunks.size() > m.ChunkCount)
        {
            ArenaChunk* c = m_Chunks.back();
            m_Chunks.pop_back();
            m_GlobalArena->FreeChunkStruct(c);
        }

        if (!m_Chunks.empty())
        {
            m_Chunks.back()->Used = m.UsedInLast;
            m_Current = m_Chunks.back();
        }
        else
        {
            m_Current = nullptr;
        }

#ifdef ARENA_DEBUG
        RecalcStats();
#endif
    }

    void Arena::Reset(bool keepOneChunk)
    {
        while (m_Chunks.size() > (keepOneChunk ? 1u : 0u))
        {
            ArenaChunk* c = m_Chunks.back();
            m_Chunks.pop_back();
            m_GlobalArena->FreeChunkStruct(c);
        }

        if (!m_Chunks.empty())
        {
            m_Chunks.back()->Used = 0;
            m_Current = m_Chunks.back();
        }
        else
        {
            m_Current = nullptr;
        }

#ifdef ARENA_DEBUG
        RecalcStats();
#endif
    }

    bool Arena::AcquireChunk(size_t minCapacity)
    {
        ArenaChunk* newChunk = m_GlobalArena->AllocateChunkStruct(minCapacity, m_Reservation);

        if (!newChunk)
        {
            return false;
        }

        m_Chunks.push_back(newChunk);
        m_Current = newChunk;

        return true;
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
        for (ArenaChunk* c : m_Chunks)
        {
            total += c->Used;
        }

        m_Used.store(total);
        size_t hw = m_HighWater.load();
        if (total > hw)
        {
            m_HighWater.store(total);
        }
    }
}
