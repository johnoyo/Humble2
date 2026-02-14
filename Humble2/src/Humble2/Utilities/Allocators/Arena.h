#pragma once

#include "MainArena.h"

namespace HBL2
{
    /**
     * @brief Arena: single-threaded fast linear allocator that uses ArenaChunk instances.
     *
     * The Arena requests chunks from MainArena. Allocations inside an Arena are
     * lock-free (simple pointer arithmetic) and intended for use by a single thread.
     */
    class Arena
    {
    public:
        Arena() = default;

        /**
         * @brief Construct an Arena bound to a MainArena and optional reservation.
         *
         * @param global Reference to the MainArena.
         * @param bytes Arena configuration.
         * @param reservation Optional PoolReservation* to prefer.
         */
        Arena(MainArena* global, size_t bytes, PoolReservation* reservation = nullptr)
        {
            Initialize(global, bytes, reservation);
        }

        /**
         * @brief Destroy the Arena and return chunks to free lists.
         */
        ~Arena()
        {
            // Return chunk structs to per-reservation or global free lists
            for (ArenaChunk* ch : m_Chunks)
            {
                m_GlobalArena->FreeChunkStruct(ch);
            }

            m_Chunks.clear();
            m_Current = nullptr;
        }

        void Initialize(MainArena* global, size_t bytes, PoolReservation* reservation = nullptr)
        {
            m_GlobalArena = global;
            m_Bytes = bytes;
            m_Reservation = reservation;
            m_Current = nullptr;
            m_NextChunkSize = bytes;

            AcquireChunk(m_NextChunkSize);

            m_Used.store(0);
            m_HighWater.store(0);
        }

        /**
         * @brief Allocate @p size bytes with @p alignment from this arena.
         *
         * Fast path: allocate from current chunk (no locks). When current chunk
         * cannot satisfy the request, the arena requests a new chunk from MainArena.
         *
         * @param size Number of bytes.
         * @param alignment Required alignment.
         * @return Pointer to allocated memory.
         * @throws std::bad_alloc if MainArena cannot provide a new chunk.
         */
        void* Alloc(size_t size, size_t alignment = alignof(std::max_align_t))
        {
            if (size == 0)
            {
                return nullptr;
            }

            ArenaChunk* ch = m_Current;
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

            // Request new chunk (may throw)
            size_t requestSize = std::max<size_t>(size + alignment, m_NextChunkSize);
            if (m_NextChunkSize < requestSize)
            {
                m_NextChunkSize = requestSize;
            }
            else
            {
                m_NextChunkSize = std::min(m_NextChunkSize * 2, requestSize);
            }

            AcquireChunk(requestSize);

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

        /**
         * @brief Contruct the provided allocated memory (by calling the ctor using placement new).
         *
         * @param allocatedMemory Pointer to allocated memory.
         * @param args Arguments to forward to ctor of type.
         * @return Pointer to constructed memory.
         */
        template<typename T, typename... Args>
        T* Construct(void* allocatedMemory, Args&&... args)
        {
            T* obj = ::new (allocatedMemory) T(std::forward<Args>(args)...);
            return obj;
        }

        /**
         * @brief Construct an array of objects of type T in the provided allocated memory
         *        by invoking their constructors using placement new.
         *
         * @tparam T Type of object to construct.
         * @tparam Args Argument types forwarded to the constructor of T.
         *
         * @param allocatedMemory Pointer to raw allocated memory with sufficient size and alignment for an array of T.
         * @param count Number of objects to construct.
         * @param args Arguments forwarded to the constructor of each T instance.
         *
         * @return Pointer to the first constructed object in the array.
         */
        template<typename T, typename... Args>
        T* ConstructArray(void* allocatedMemory, size_t count, Args&&... args)
        {
            T* p = static_cast<T*>(allocatedMemory);
            for (size_t i = 0; i < count; i++)
            {
                ::new (static_cast<void*>(p + i)) T(std::forward<Args>(args)...);
            }
            return p;
        }

        /**
         * @brief Allocate and construct object of type T in scratch area.
         *
         * @param args Arguments to forward to ctor of type.
         * @return Pointer to allocated and constructed memory.
         */
        template<typename T, typename... Args>
        T* AllocConstruct(Args&&... args)
        {
            void* mem = Alloc(sizeof(T), alignof(T));
            T* obj = ::new (mem) T(std::forward<Args>(args)...);
            return obj;
        }

        /**
         * @brief Destruct an object of type T inside the arena (call the dtor of the object).
         *
         * @param object Pointer to the object.
         */
        template<typename T>
        void Destruct(T* object)
        {
            try { object->~T(); }
            catch (...) {}
        }

        /**
         * @brief Mark representing current arena state for restore.
         */
        struct Marker
        {
            size_t ChunkCount;
            size_t UsedInLast;
        };

        /**
         * @brief Capture a mark for later restore.
         *
         * @return Mark snapshot.
         */
        Marker Mark() const
        {
            Marker m{};
            m.ChunkCount = m_Chunks.size();
            m.UsedInLast = m_Chunks.empty() ? 0 : m_Chunks.back()->Used;
            return m;
        }

        /**
         * @brief Restore to a previous mark. Chunks allocated after the mark are returned to free lists.
         *
         * @param m Mark returned from mark().
         */
        void Restore(const Marker& m)
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

        /**
         * @brief Reset arena: return all but optionally keep one chunk.
         */
        void Reset(bool keepOneChunk = true)
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

        /**
         * @brief Current used bytes inside this arena (debug).
         */
        size_t UsedBytes() const { return m_UsedBeforeReset.load(); }

        /**
         * @brief Total available bytes inside this arena (debug).
         */
        size_t TotalBytes() const { return m_Bytes; }

        /**
         * @brief High-water mark for this arena (debug).
         */
        size_t HighWater() const { return m_HighWater.load(); }

    private:
        /**
         * @brief Acquire a chunk for the arena.
         *
         * Try reservation free list, then global free list, then allocate a new chunk
         * (placement-new in META and carve payload from DATA).
         */
        void AcquireChunk(size_t minCapacity)
        {
            ArenaChunk* newChunk = m_GlobalArena->AllocateChunkStruct(minCapacity, m_Reservation);
            m_Chunks.push_back(newChunk);
            m_Current = newChunk;
        }

        void UpdateStats(size_t delta)
        {
            size_t prev = m_Used.fetch_add(delta);
            size_t cur = prev + delta;
            size_t oldhw = m_HighWater.load();
            while (cur > oldhw && !m_HighWater.compare_exchange_weak(oldhw, cur)) {}
        }

        void RecalcStats()
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

    private:
        MainArena* m_GlobalArena = nullptr;
        size_t m_Bytes = 0;
        PoolReservation* m_Reservation = nullptr;
        std::vector<ArenaChunk*> m_Chunks;
        ArenaChunk* m_Current = nullptr;
        size_t m_NextChunkSize = 0;

        std::atomic<size_t> m_Used{ 0 };
        std::atomic<size_t> m_UsedBeforeReset{ 0 };
        std::atomic<size_t> m_HighWater{ 0 };
    };
}