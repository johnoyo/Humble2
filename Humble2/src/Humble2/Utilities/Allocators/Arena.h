// ArenaAllocator.hpp
// -----------------------------------------------------------------------------
// Strict Preallocated Global Arena Allocator (v3) with per-reservation free lists
// -----------------------------------------------------------------------------
//
// Main features & logic (concise):
//  - Single contiguous preallocated region split into META and DATA areas.
//  - All metadata (PoolReservation, ArenaChunk structs) placement-new'd in META.
//  - All chunk payloads carved from DATA; no runtime malloc fallback (strict).
//  - Per-reservation free lists for chunk reuse (mutex-protected).
//  - Global free list for chunks without reservation.
//  - Arena: ultra-fast bump allocator intended for single-thread use (no locks).
//  - ScratchScope: thin RAII wrapper (mark/restore) for temporary allocations.
//  - Debug statistics (ARENA_DEBUG) for meta/data carved bytes and per-arena usage.
//  - Strict failure: allocation that cannot be satisfied throws std::bad_alloc.
//
// Usage:
//  - #include "ArenaAllocator.hpp"
//  - Create GlobalArena once (total_bytes, meta_bytes).
//  - Optionally Reserve slices: global.Reserve("Render", size).
//  - Create per-thread/subsystem Arena(global, config, reservation).
//  - Use ScratchScope(arena) for temporary allocations.
//
// -----------------------------------------------------------------------------

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <new>
#include <stdexcept>

/// Uncomment to enable debug statistics and messages.
#define ARENA_DEBUG

#ifdef ARENA_DEBUG
# define ADBG(fmt, ...) std::fprintf(stderr, (fmt), ##__VA_ARGS__)
#else
# define ADBG(fmt, ...) ((void)0)
#endif

namespace HBL2
{
    static inline size_t AlignUp(size_t value, size_t alignment)
    {
        assert((alignment & (alignment - 1)) == 0);
        size_t mask = alignment - 1;
        return (value + mask) & ~mask;
    }

    /**
     * @brief Reservation of a contiguous slice in the global data region.
     *
     * PoolReservation is placement-new'd inside the META region. It holds a
     * per-reservation free list and a mutex protecting that list and the reservation offset.
     */
    struct PoolReservation
    {
        const char* Name = nullptr;
        size_t Start;                 // offset within DATA region (relative to DATA base)
        size_t Size;                  // total reserved bytes
        size_t Offset;                // current carve offset inside reservation
        std::vector<class ArenaChunk*> FreeChunks; // reservation-local free list
        std::mutex FreeChunksMutex;   // protect FreeChunks and (optionally) Offset modifications

        PoolReservation()
            : Name(), Start(SIZE_MAX), Size(0), Offset(0)
        {
            FreeChunks.reserve(256);
        }
    };

    /**
     * @brief A contiguous chunk used by an Arena to perform bump allocations.
     *
     * ArenaChunk structs are placement-new'd inside the META region. The Data pointer points
     * into the DATA region and is owned by the chunk (no external free). Capacity is fixed.
     */
    struct ArenaChunk
    {
        uint8_t* Data;           // chunk payload pointer (in DATA region)
        size_t Capacity;         // payload size
        size_t Used;             // bump pointer
        PoolReservation* Reservation; // owning reservation at allocation time

        ArenaChunk(uint8_t* d, size_t c, PoolReservation* r)
            : Data(d), Capacity(c), Used(0), Reservation(r)
        {
        }

        /**
         * @brief Check if this chunk can satisfy an allocation of @p size with @p alignment.
         *
         * @param size Allocation size in bytes.
         * @param alignment Required alignment.
         * @return true when allocation fits, false otherwise.
         */
        inline bool HasSpace(size_t size, size_t alignment) const
        {
            uintptr_t base = reinterpret_cast<uintptr_t>(Data);
            uintptr_t cur = base + Used;
            uintptr_t aligned = AlignUp(cur, alignment);
            size_t offset = static_cast<size_t>(aligned - base);
            return (offset + size) <= Capacity;
        }
    };

    /**
     * @brief GlobalArena owns the contiguous preallocated memory and metadata region.
     *
     * The buffer is split into META and DATA regions. All metadata objects (reservations,
     * chunk structs) are placement-new'd into META. Chunk payloads are carved from DATA.
     *
     * Strict policy: no malloc fallback. If meta or data region cannot satisfy a request,
     * functions throw std::bad_alloc.
     */
    class GlobalArena
    {
    public:
        GlobalArena() = default;

        /**
         * @brief Construct a GlobalArena with a contiguous buffer.
         *
         * @param totalBytes Total bytes to reserve for META + DATA.
         * @param metaBytes  Bytes reserved for META (must be >0 and < total_bytes).
         */
        GlobalArena(size_t totalBytes, size_t metaBytes)
        {
            Initialize(totalBytes, metaBytes);
        }

        /**
         * @brief Destroy GlobalArena, call destructors for placement-new'd metadata.
         */
        ~GlobalArena()
        {
            // Destroy reservations (placement-new)
            for (auto& kv : m_Reservations)
            {
                PoolReservation* r = kv.second;
                if (r)
                {
                    r->~PoolReservation();
                }
            }
            m_Reservations.clear();

            // Destroy chunk structs in global free list (placement-new)
            {
                std::lock_guard<std::mutex> lk(m_ChunkFreeListMutex);
                for (ArenaChunk* ch : m_ChunkFreeList)
                {
                    if (ch) ch->~ArenaChunk();
                }
                m_ChunkFreeList.clear();
            }

            if (m_Mem)
            {
                std::free(m_Mem);
                m_Mem = nullptr;
            }
        }

        /**
         * @brief Construct a GlobalArena with a contiguous buffer.
         *
         * @param totalBytes Total bytes to reserve for META + DATA.
         * @param metaBytes  Bytes reserved for META (must be >0 and < totalBytes).
         */
        void Initialize(size_t totalBytes, size_t metaBytes)
        {
            m_TotalBytes = totalBytes;

            if (metaBytes == 0 || metaBytes >= totalBytes)
            {
                throw std::invalid_argument("metaBytes must be >0 and < totalBytes");
            }

            m_Mem = static_cast<uint8_t*>(std::malloc(m_TotalBytes));
            if (!m_Mem)
            {
                throw std::bad_alloc();
            }

            m_MetaBase = m_Mem;
            size_t metaRegionActual = AlignUp(metaBytes, alignof(std::max_align_t));
            m_DataBase = m_Mem + metaRegionActual;
            m_MetaSize = metaRegionActual;
            m_DataSize = m_TotalBytes - metaRegionActual;

            m_MetaOffset = 0;
            m_DataOffset = 0;

            m_MetaCarved.store(0);
            m_DataCarved.store(0);

            m_Reservations.max_load_factor(0.75f);
            m_Reservations.reserve(64);

            m_ChunkFreeList.reserve(512);
        }

        /**
         * @brief Reserve a named slice of the global DATA region.
         *
         * @param name Human-readable reservation name.
         * @param bytes Number of bytes to reserve.
         * @return Pointer to the PoolReservation (placement-new'd in META).
         * @throws std::bad_alloc when meta or data region cannot satisfy request.
         */
        PoolReservation* Reserve(std::string_view name, size_t bytes)
        {
            // Lock meta for the entire reservation creation + map insertion.
            std::unique_lock<std::mutex> lk_meta(m_MetaMutex);

            // Check if it already exists (now safe).
            auto it = m_Reservations.find(name);
            if (it != m_Reservations.end())
            {
                return it->second;
            }

            // Intern name into META
            const char* internedName = InternString(name.data(), name.size());

            void* metaPtr = AllocMetaNoLock(sizeof(PoolReservation), alignof(PoolReservation));
            if (!metaPtr)
            {
                ADBG("GlobalArena ERROR: meta region exhausted when creating reservation '%s'\n", name.data());
                throw std::bad_alloc();
            }

            PoolReservation* r = ::new (metaPtr) PoolReservation();
            r->Name = internedName;

            // Carve DATA region under data lock.
            {
                std::lock_guard<std::mutex> lk_data(m_DataMutex);
                size_t alignedDataOff = AlignUp(m_DataOffset, alignof(std::max_align_t));
                if (alignedDataOff + bytes > m_DataSize)
                {
                    r->~PoolReservation();
                    ADBG("GlobalArena ERROR: data region insufficient for reservation '%s' (%zu bytes)\n", name.data(), bytes);
                    throw std::bad_alloc();
                }

                r->Start = alignedDataOff;
                r->Size = bytes;
                r->Offset = 0;

                m_DataOffset = alignedDataOff + bytes;
#ifdef ARENA_DEBUG
                m_DataCarved.fetch_add(bytes);
#endif
            }

            // Insert into map while meta lock still held.
            m_Reservations.emplace(r->Name, r);

#ifdef ARENA_DEBUG
            m_MetaCarved.fetch_add(sizeof(PoolReservation));
#endif
            return r;
        }


        /**
         * @brief Get an existing reservation by name.
         *
         * @param name Reservation name.
         * @return PoolReservation* or nullptr if not found.
         */
        PoolReservation* GetReservation(const std::string& name)
        {
            std::lock_guard<std::mutex> lk(m_MetaMutex);
            auto it = m_Reservations.find(name);
            return (it == m_Reservations.end()) ? nullptr : it->second;
        }

        /**
         * @brief Carve bytes from reservation or global DATA region.
         *
         * Thread-safe. Throws std::bad_alloc if not enough space.
         *
         * @param bytes Number of bytes requested.
         * @param r Optional reservation to prefer.
         * @return Pointer to carved payload in DATA region.
         */
        uint8_t* CarveData(size_t bytes, PoolReservation* r = nullptr)
        {
            std::lock_guard<std::mutex> lk(m_DataMutex);

            if (r)
            {
                size_t aligned = AlignUp(r->Offset, alignof(std::max_align_t));
                if (aligned + bytes <= r->Size)
                {
                    uint8_t* p = m_DataBase + r->Start + aligned;
                    r->Offset = aligned + bytes;
#ifdef ARENA_DEBUG
                    m_DataCarved.fetch_add(bytes);
#endif
                    return p;
                }
                // fall-through to global carve
            }

            size_t alignedGlobal = AlignUp(m_DataOffset, alignof(std::max_align_t));
            if (alignedGlobal + bytes <= m_DataSize)
            {
                uint8_t* p = m_DataBase + alignedGlobal;
                m_DataOffset = alignedGlobal + bytes;
#ifdef ARENA_DEBUG
                m_DataCarved.fetch_add(bytes);
#endif
                return p;
            }

            ADBG("GlobalArena ERROR: data region exhausted (requested %zu bytes)\n", bytes);
            throw std::bad_alloc();
        }

        /**
         * @brief Allocate @p bytes from META region for placement-new use.
         *
         * @param bytes Number of bytes to allocate.
         * @param alignment Required alignment.
         * @return pointer in META region or nullptr when META exhausted.
         */
        void* AllocMetaNoLock(size_t bytes, size_t alignment = alignof(std::max_align_t))
        {
            size_t aligned = AlignUp(m_MetaOffset, alignment);
            if (aligned + bytes <= m_MetaSize)
            {
                void* p = m_MetaBase + aligned;
                m_MetaOffset = aligned + bytes;
#ifdef ARENA_DEBUG
                m_MetaCarved.fetch_add(bytes);
#endif
                return p;
            }
            return nullptr;
        }

        /**
         * @brief Allocate @p bytes from META region for placement-new use.
         *
         * @param bytes Number of bytes to allocate.
         * @param alignment Required alignment.
         * @return pointer in META region or nullptr when META exhausted.
         */
        void* AllocMeta(size_t bytes, size_t alignment = alignof(std::max_align_t))
        {
            std::lock_guard<std::mutex> lk(m_MetaMutex);
            return AllocMetaNoLock(bytes, alignment);
        }

        /**
         * @brief Allocate or reuse an ArenaChunk struct plus payload.
         *
         * Preference order:
         *  1) reservation free list (if reservation provided)
         *  2) global free list
         *  3) allocate new chunk struct in META and carve payload in DATA
         *
         * @param payload_capacity desired chunk capacity
         * @param reservation optional reservation to prefer
         * @return ArenaChunk* (placement-new'd in META)
         * @throws std::bad_alloc when unable to allocate
         */
        ArenaChunk* AllocateChunkStruct(size_t payload_capacity, PoolReservation* reservation)
        {
            // Reservation-local reuse
            if (reservation)
            {
                std::lock_guard<std::mutex> lkRes(reservation->FreeChunksMutex);
                for (size_t i = 0; i < reservation->FreeChunks.size(); ++i)
                {
                    ArenaChunk* candidate = reservation->FreeChunks[i];
                    if (candidate->Capacity >= payload_capacity)
                    {
                        reservation->FreeChunks[i] = reservation->FreeChunks.back();
                        reservation->FreeChunks.pop_back();
                        candidate->Used = 0;
                        candidate->Reservation = reservation;
                        return candidate;
                    }
                }
            }

            // Global free list reuse
            {
                std::lock_guard<std::mutex> lkGlobal(m_ChunkFreeListMutex);
                for (size_t i = 0; i < m_ChunkFreeList.size(); ++i)
                {
                    ArenaChunk* candidate = m_ChunkFreeList[i];
                    if (candidate->Capacity >= payload_capacity)
                    {
                        m_ChunkFreeList[i] = m_ChunkFreeList.back();
                        m_ChunkFreeList.pop_back();
                        candidate->Used = 0;
                        candidate->Reservation = reservation;
                        return candidate;
                    }
                }
            }

            // Allocate new chunk struct in META region
            void* structMem = AllocMeta(sizeof(ArenaChunk), alignof(ArenaChunk));
            if (!structMem)
            {
                ADBG("GlobalArena ERROR: meta region exhausted while allocating ArenaChunk\n");
                throw std::bad_alloc();
            }

            // Carve payload (may throw)
            uint8_t* payload = CarveData(payload_capacity, reservation);

            // Construct chunk in META
            ArenaChunk* ch = ::new (structMem) ArenaChunk(payload, payload_capacity, reservation);
            return ch;
        }

        /**
         * @brief Return a chunk to appropriate free list.
         *
         * If the chunk has a Reservation, push into that reservation's FreeChunks,
         * otherwise push into the global chunk free list.
         *
         * @param ch ArenaChunk* to return.
         */
        void FreeChunkStruct(ArenaChunk* ch)
        {
            if (!ch)
            {
                return;
            }

            ch->Used = 0;

            if (ch->Reservation)
            {
                std::lock_guard<std::mutex> lkRes(ch->Reservation->FreeChunksMutex);
                ch->Reservation->FreeChunks.push_back(ch);
            }
            else
            {
                std::lock_guard<std::mutex> lkGlobal(m_ChunkFreeListMutex);
                m_ChunkFreeList.push_back(ch);
            }
        }

        /**
         * @brief META carved bytes (debug).
         */
        size_t MetaCarved() const { return m_MetaCarved.load(); }

        /**
         * @brief DATA carved bytes (debug).
         */
        size_t DataCarved() const { return m_DataCarved.load(); }

        /**
         * @brief META region size (debug).
         */
        size_t MetaSize() const { return m_MetaSize; }

        /**
         * @brief DATA region size (debug).
         */
        size_t DataSize() const { return m_DataSize; }

        float GetFullPercentage()
        {
            return ((float)m_DataOffset / (float)m_DataSize) * 100.f;
        }

    private:
        const char* InternString(const char* str, size_t len)
        {
            // +1 for null terminator
            char* mem = static_cast<char*>(AllocMetaNoLock(len + 1, alignof(char)));
            if (!mem) throw std::bad_alloc();

            std::memcpy(mem, str, len);
            mem[len] = '\0';
            return mem;
        }

    private:
        uint8_t* m_Mem = nullptr;

        // META region
        uint8_t* m_MetaBase = nullptr;
        size_t  m_MetaSize = 0;
        size_t  m_MetaOffset = 0;
        std::mutex m_MetaMutex;

        // DATA region
        uint8_t* m_DataBase = nullptr;
        size_t  m_DataSize = 0;
        size_t  m_DataOffset = 0;
        std::mutex m_DataMutex;

        // Free-lists
        std::mutex m_ChunkFreeListMutex;
        std::vector<ArenaChunk*> m_ChunkFreeList;

        // Reservations map (name -> PoolReservation*)
        std::unordered_map<std::string_view, PoolReservation*> m_Reservations;

        size_t m_TotalBytes = 0;

        std::atomic<size_t> m_MetaCarved{ 0 };
        std::atomic<size_t> m_DataCarved{ 0 };
    };

    /**
     * @brief Arena: single-threaded fast linear allocator that uses ArenaChunk instances.
     *
     * The Arena requests chunks from GlobalArena. Allocations inside an Arena are
     * lock-free (simple pointer arithmetic) and intended for use by a single thread.
     */
    class Arena
    {
    public:
        Arena() = default;

        /**
         * @brief Construct an Arena bound to a GlobalArena and optional reservation.
         *
         * @param global Reference to the GlobalArena.
         * @param bytes Arena configuration.
         * @param reservation Optional PoolReservation* to prefer.
         */
        Arena(GlobalArena* global, size_t bytes, PoolReservation* reservation = nullptr)
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

        void Initialize(GlobalArena* global, size_t bytes, PoolReservation* reservation = nullptr)
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
         * cannot satisfy the request, the arena requests a new chunk from GlobalArena.
         *
         * @param size Number of bytes.
         * @param alignment Required alignment.
         * @return Pointer to allocated memory.
         * @throws std::bad_alloc if GlobalArena cannot provide a new chunk.
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
        size_t UsedBytes() const { return m_Used.load(); }

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
        GlobalArena* m_GlobalArena = nullptr;
        size_t m_Bytes = 0;
        PoolReservation* m_Reservation = nullptr;
        std::vector<ArenaChunk*> m_Chunks;
        ArenaChunk* m_Current = nullptr;
        size_t m_NextChunkSize = 0;

        std::atomic<size_t> m_Used{ 0 };
        std::atomic<size_t> m_HighWater{ 0 };
    };

    /**
     * @brief Thin RAII wrapper that marks an Arena on construction and restores on destruction.
     */
    class ScratchArena
    {
    public:
        /**
         * @brief Construct and capture arena mark.
         *
         * @param arena Reference to an existing Arena.
         */
        explicit ScratchArena(Arena& arena)
            : m_Arena(arena), m_Mark(arena.Mark())
        {
        }

        /**
         * @brief Restore arena to the captured mark on destruction.
         */
        ~ScratchArena()
        {
            m_Arena.Restore(m_Mark);
        }

        /**
         * @brief Allocate temporary bytes from the underlying arena.
         *
         * @param bytes Number of bytes to allocate.
         * @param align Alignment requirement.
         * @return Pointer to allocated memory.
         */
        void* Alloc(size_t bytes, size_t align = alignof(std::max_align_t))
        {
            return m_Arena.Alloc(bytes, align);
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
            return m_Arena.Construct<T>(allocatedMemory, std::forward<Args>(args)...);
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
            return m_Arena.AllocConstruct<T>(std::forward<Args>(args)...);
        }

        /**
         * @brief Destruct an object of type T inside the arena (call the dtor of the object).
         *
         * @param object Pointer to the object.
         */
        template<typename T>
        void Destruct(T* object)
        {
            m_Arena.Destruct(object);
        }

        inline Arena* GetArena() { return &m_Arena; }

    private:
        Arena& m_Arena;
        Arena::Marker m_Mark;
    };

    /**
     * @brief PoolArena: fixed-size block allocator with lock-free free list (MPMC).
     *
     * - Thread-safe, lock-free allocations/frees (index-based tagged stack).
     * - Strict: no fallback malloc; exhaustion throws std::bad_alloc.
     * - Memory comes from GlobalArena DATA (optionally from a PoolReservation).
     * - Metadata (free-list "next" array) is placement-new'd in GlobalArena META.
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

        PoolArena(GlobalArena* global, size_t totalBytes, size_t blockSize, PoolReservation* reservation = nullptr)
        {
            Initialize(global, totalBytes, blockSize, reservation);
        }

        ~PoolArena() = default; // No chunk structs to return; underlying DATA is owned by GlobalArena.

        /**
         * @brief Initialize the pool.
         *
         * @param global       GlobalArena backing store.
         * @param totalBytes   Total bytes to carve for pool storage.
         * @param blockSize    Fixed block size returned by Alloc (aligned up internally).
         * @param reservation  Optional reservation to carve from.
         */
        void Initialize(GlobalArena* global, size_t totalBytes, size_t blockSize, PoolReservation* reservation = nullptr, size_t blockAlign = 16)
        {
            if (!global) throw std::invalid_argument("PoolArena::Initialize: global is null");
            if (totalBytes == 0) throw std::invalid_argument("PoolArena::Initialize: totalBytes == 0");
            if (blockSize == 0) throw std::invalid_argument("PoolArena::Initialize: blockSize == 0");

            // Enforce power-of-two alignment
            if ((blockAlign & (blockAlign - 1)) != 0)
                throw std::invalid_argument("PoolArena::Initialize: blockAlign must be power of two");

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
        GlobalArena* m_GlobalArena = nullptr;
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

    // Thread-local scratch arena (per-thread fast allocator)
    inline thread_local Arena* g_ThreadScratchArena = nullptr;

    /**
    * @brief Gets the thread-local scratch arena.
    * If not set, returns nullptr — user must initialize before use.
    */
    inline Arena* GetThreadScratchArena()
    {
        return g_ThreadScratchArena;
    }

    /**
    * @brief Sets the thread-local scratch arena (usually once per thread).
    */
    inline void SetThreadScratchArena(Arena* arena)
    {
        g_ThreadScratchArena = arena;
    }
}