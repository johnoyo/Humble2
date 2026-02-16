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
//  - Create MainArena once (total_bytes, meta_bytes).
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

#define ARENA_DEBUG

namespace HBL2
{
    static constexpr size_t AlignUp(size_t value, size_t alignment)
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
     * @brief MainArena owns the contiguous preallocated memory and metadata region.
     *
     * The buffer is split into META and DATA regions. All metadata objects (reservations,
     * chunk structs) are placement-new'd into META. Chunk payloads are carved from DATA.
     *
     * Strict policy: no malloc fallback. If meta or data region cannot satisfy a request,
     * functions throw std::bad_alloc.
     */
    class MainArena
    {
    public:
        MainArena() = default;

        /**
         * @brief Construct a MainArena with a contiguous buffer.
         *
         * @param totalBytes Total bytes to reserve for META + DATA.
         * @param metaBytes  Bytes reserved for META (must be >0 and < total_bytes).
         */
        MainArena(size_t totalBytes, size_t metaBytes)
        {
            Initialize(totalBytes, metaBytes);
        }

        /**
         * @brief Destroy MainArena, call destructors for placement-new'd metadata.
         */
        ~MainArena()
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
                    if (ch)
                    {
                        ch->~ArenaChunk();
                    }
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
         * @brief Construct a MainArena with a contiguous buffer.
         *
         * @param totalBytes Total bytes to reserve for META + DATA.
         * @param metaBytes  Bytes reserved for META (must be >0 and < totalBytes).
         */
        void Initialize(size_t totalBytes, size_t metaBytes)
        {
            m_TotalBytes = totalBytes;

            if (metaBytes == 0 || metaBytes >= totalBytes)
            {
                HBL2_CORE_FATAL("MainArena ERROR: meta bytes must be > 0 and <= than totalBytes.");
                exit(-1);
            }

            m_Mem = static_cast<uint8_t*>(std::malloc(m_TotalBytes));
            if (!m_Mem)
            {
                HBL2_CORE_FATAL("MainArena ERROR: could not allocate {} bytes.", m_TotalBytes);
                exit(-1);
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
            if (!internedName)
            {
                HBL2_CORE_ERROR("MainArena ERROR: Could not allocate string in meta region, reservation {} failed.", name.data());
                return nullptr;
            }

            void* metaPtr = AllocMetaNoLock(sizeof(PoolReservation), alignof(PoolReservation));
            if (!metaPtr)
            {
                HBL2_CORE_ERROR("MainArena ERROR: Meta region exhausted when creating reservation '{}'", name.data());
                return nullptr;
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
                    HBL2_CORE_ERROR("MainArena ERROR: Data region insufficient for reservation '{}' ({} bytes)", name.data(), bytes);
                    return nullptr;
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

            HBL2_CORE_ERROR("GlobalArena ERROR: Data region exhausted (requested {} bytes)", bytes);
            return nullptr;
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
                HBL2_CORE_ERROR("GlobalArena ERROR: meta region exhausted while allocating ArenaChunk");
                return nullptr;
            }

            // Carve payload.
            uint8_t* payload = CarveData(payload_capacity, reservation);

            if (!payload)
            {
                return nullptr;
            }

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

        uint8_t* DataBase() const { return m_DataBase; }

        float GetFullPercentage()
        {
            return ((float)m_DataOffset / (float)m_DataSize) * 100.f;
        }

    private:
        const char* InternString(const char* str, size_t len)
        {
            // +1 for null terminator
            char* mem = static_cast<char*>(AllocMetaNoLock(len + 1, alignof(char)));
            if (!mem)
            {
                return nullptr;
            }

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
}