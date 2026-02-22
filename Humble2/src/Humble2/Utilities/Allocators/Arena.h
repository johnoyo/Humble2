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
    class HBL2_API Arena
    {
    public:
        Arena() = default;

        /**
         * @brief Destroy the Arena and return chunks to free lists.
         */
        ~Arena();

        /**
         * @brief Construct an Arena bound to a MainArena and optional reservation.
         *
         * @param global Reference to the MainArena.
         * @param bytes Arena configuration.
         * @param reservation Optional PoolReservation* to prefer.
         */
        void Initialize(MainArena* global, size_t bytes, PoolReservation* reservation);

        /**
         * @brief Destroy the Arena and return chunks to free lists.
         */
        void Destroy();

        /**
         * @brief Allocate @p size bytes with @p alignment from this arena.
         *
         * Fast path: allocate from current chunk (no locks). When current chunk
         * cannot satisfy the request, the arena requests a new chunk from MainArena reservation.
         *
         * @param size Number of bytes.
         * @param alignment Required alignment.
         * @return Pointer to allocated memory.
         */
        void* Alloc(size_t size, size_t alignment = alignof(std::max_align_t));

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

            if (!mem)
            {
                return nullptr;
            }

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
        Marker Mark() const;

        /**
         * @brief Restore to a previous mark. Chunks allocated after the mark are returned to free lists.
         *
         * @param m Mark returned from mark().
         */
        void Restore(const Marker& m);

        /**
         * @brief Reset arena: return all but optionally keep one chunk.
         */
        void Reset(bool keepOneChunk = true);

        /**
         * @brief Current used bytes inside this arena (debug).
         */
        inline size_t UsedBytes() const { return m_UsedBeforeReset.load(); }

        /**
         * @brief Total available bytes inside this arena (debug).
         */
        inline size_t TotalBytes() const { return m_Bytes; }

        /**
         * @brief High-water mark for this arena (debug).
         */
        inline size_t HighWater() const { return m_HighWater.load(); }

    private:
        bool AcquireChunk(size_t minCapacity);

        void UpdateStats(size_t delta);

        void RecalcStats();

    private:
        MainArena* m_GlobalArena = nullptr;
        size_t m_Bytes = 0;
        PoolReservation* m_Reservation = nullptr;
        std::vector<ArenaChunk*> m_Chunks;
        ArenaChunk* m_Current = nullptr;
        size_t m_NextChunkSize = 0;
        bool m_Destructed = false;

        std::atomic<size_t> m_Used{ 0 };
        std::atomic<size_t> m_UsedBeforeReset{ 0 };
        std::atomic<size_t> m_HighWater{ 0 };
    };
}