#pragma once

#include "Base.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <new>
#include <algorithm>
#include <thread>

namespace HBL2
{
    static constexpr size_t AlignUp(size_t value, size_t alignment)
    {
        HBL2_CORE_ASSERT((alignment & (alignment - 1)) == 0, "Bad alignment!");
        size_t mask = alignment - 1;
        return (value + mask) & ~mask;
    }

    static inline void CopyName(char* dst, size_t dstCap, std::string_view name)
    {
        const std::size_t len = std::min(name.size(), dstCap - 1);
        std::memcpy(dst, name.data(), len);
        dst[len] = '\0';
    }

    static constexpr uint32_t INVALID32 = 0xFFFFFFFFu;

    static constexpr uint32_t NUM_TOP_BINS = 32;
    static constexpr uint32_t BINS_PER_LEAF = 8;
    static constexpr uint32_t TOP_BINS_INDEX_SHIFT = 3;
    static constexpr uint32_t LEAF_BINS_INDEX_MASK = 0x7;
    static constexpr uint32_t NUM_LEAF_BINS = NUM_TOP_BINS * BINS_PER_LEAF;

    typedef uint32_t NodeIndex;

    struct ArenaAllocation
    {
        static constexpr uint32_t NO_SPACE = 0xffffffff;

        uint32_t offset = NO_SPACE;
        NodeIndex metadata = NO_SPACE; // internal: node index
    };

    struct StorageReport
    {
        uint32_t totalFreeSpace;
        uint32_t largestFreeRegion;
    };

    struct StorageReportFull
    {
        struct Region
        {
            uint32_t size;
            uint32_t count;
        };

        Region freeRegions[NUM_LEAF_BINS];
    };

    /**
     * @brief ...
     *
     * @note Reservations are NOT thread-safe for allocation.
     */
    struct PoolReservation
    {
        static constexpr std::size_t MaxName = 64;
        char Name[MaxName]{};

        // Reservation payload in DATA heap
        size_t Start = SIZE_MAX; // offset within DATA region (relative to DATA base)
        size_t Size = 0;         // total reserved bytes
        size_t Offset = 0;       // bump offset inside reservation

        std::atomic<uint32_t> RefCount{ 0 };

        // Chunk struct reuse list (structs live in META)
        std::vector<class ArenaChunk*> FreeChunks;
        std::mutex FreeChunksMutex;

        // Internal handle to free reservation backing memory from DATA heap.
        // (User never touches this; required for Release().)
        uint32_t BackingOffset = 0xFFFFFFFFu;
        uint32_t BackingMeta = 0xFFFFFFFFu;

        PoolReservation(std::string_view name)
        {
            FreeChunks.reserve(256);
            CopyName(Name, MaxName, name);
        }

        bool IsLive() const { return Start != SIZE_MAX && Size != 0; }

#ifdef ARENA_DEBUG
        // Debug safety rails
        std::atomic<uint32_t> LiveChunks{ 0 };
        uint32_t Generation = 1;
        std::thread::id OwnerThread{};
        bool OwnerSet = false;

        void ResetDebugState()
        {
            LiveChunks.store(0, std::memory_order_relaxed);
            Generation++;
            OwnerThread = {};
            OwnerSet = false;
        }

        void DebugClaimThread()
        {
            if (!OwnerSet)
            {
                OwnerThread = std::this_thread::get_id();
                OwnerSet = true;
            }
            else
            {
                HBL2_CORE_ASSERT(OwnerThread == std::this_thread::get_id(), "PoolReservation used from multiple threads");
            }
        }
#endif
    };

    /**
     * @brief ...
     */
    struct ArenaChunk
    {
        uint8_t* Data = nullptr;
        size_t Capacity = 0;
        size_t Used = 0;
        PoolReservation* Reservation = nullptr;

#ifdef ARENA_DEBUG
        uint32_t ReservationGeneration = 0;
#endif

        ArenaChunk(uint8_t* d, size_t c, PoolReservation* r)
            : Data(d), Capacity(c), Used(0), Reservation(r)
        {
#ifdef ARENA_DEBUG
            ReservationGeneration = r ? r->Generation : 0;
#endif
        }

        inline bool HasSpace(size_t size, size_t alignment) const
        {
            uintptr_t base = reinterpret_cast<uintptr_t>(Data);
            uintptr_t cur = base + Used;
            uintptr_t aligned = AlignUp(cur, alignment);
            size_t offset = static_cast<size_t>(aligned - base);
            return (offset + size) <= Capacity;
        }

#ifdef ARENA_DEBUG
        inline void DebugValidateLive() const
        {
            if (!Reservation) return;
            HBL2_CORE_ASSERT(Reservation->IsLive(), "Using a chunk from a released reservation");
            HBL2_CORE_ASSERT(ReservationGeneration == Reservation->Generation, "Using a chunk from an old reservation generation");
        }
#endif
    };

    /**
     * @brief ...
     *
     * @note Inspired by Sebastian Aaltonens' OffsetAllocator and adjusted to use pointers API (https://github.com/sebbbi/OffsetAllocator).
     */
    class HBL2_API MainArena
    {
    public:
        MainArena() = default;
        MainArena(size_t totalBytes, size_t metaBytes);

        ~MainArena();

        void Initialize(size_t totalBytes, size_t metaBytes);

        /**
         * @brief Returns a reservation of the specified size.
         *
         * @note Reservations are NOT thread-safe for allocation. Should only be used by one thread.
         *
         * @param name Debug name.
         * @param bytes Number of bytes that reservation can hold.
         * @return A pointer to the reservation.
         */
        PoolReservation* Reserve(std::string_view name, size_t bytes);

        /**
         * @brief Releases the memory of the reservation and appends it in the free list.
         *
         * @note Caller must guarantee no live Arenas/chunks still reference this reservation.
         *
         * @param r The reservation to release.
         */
        void TryRelease(PoolReservation* r);

        void Reset();

        // Strict reservation carve (used for reservation chunk payloads)
        uint8_t* CarveData(size_t bytes, PoolReservation* r);

        void* AllocMetaNoLock(size_t bytes, size_t alignment = alignof(std::max_align_t));

        void* AllocMeta(size_t bytes, size_t alignment = alignof(std::max_align_t));

        ArenaChunk* AllocateChunkStruct(size_t payload_capacity, PoolReservation* reservation);

        void FreeChunkStruct(ArenaChunk* ch);

        bool AllocateReservationBacking(PoolReservation* r, size_t bytes);

        // Debug helpers
        inline size_t MetaCarved() const { return m_MetaCarved.load(std::memory_order_relaxed); }
        inline size_t DataCarved() const { return m_DataCarved.load(std::memory_order_relaxed); }
        inline size_t MetaSize() const { return m_MetaSize; }
        inline size_t DataSize() const { return m_DataSize; }
        inline size_t DataInUse() const { return m_DataInUse.load(std::memory_order_relaxed); }
        inline uint8_t* DataBase() const { return m_DataBase; }
        inline float GetFullPercentage() { const float denom = (float)DataSize(); return denom > 0.0f ? (DataInUse() / denom * 100.f) : 0.0f; }

        StorageReport GetStorageReport() const;
        StorageReportFull GetStorageReportFull() const;
    private:
        // Raw memory
        uint8_t* m_Mem = nullptr;
        size_t   m_TotalBytes = 0;

        // META region (monotonic)
        uint8_t* m_MetaBase = nullptr;
        size_t   m_MetaSize = 0;
        size_t   m_MetaOffset = 0;
        std::mutex m_MetaMutex;

        // DATA region base/size
        uint8_t* m_DataBase = nullptr;
        size_t   m_DataSize = 0;

        // Reservations array
        std::vector<PoolReservation*> m_Reservations;
        std::vector<PoolReservation*> m_FreeReservations;

        // Debug counters (kept consistent in all builds)
        std::atomic<size_t> m_MetaCarved{ 0 };
        std::atomic<size_t> m_DataInUse{ 0 };
        std::atomic<size_t> m_DataCarved{ 0 };

        // Anonymous reservation counter
        std::atomic<uint64_t> m_AnonCounter{ 0 };

        // DATA heap metadata/state
        std::mutex m_DataHeapMutex;

        // 
        uint32_t InsertNodeIntoBin(uint32_t size, uint32_t dataOffset);
        void RemoveNodeFromBin(uint32_t nodeIndex);
        ArenaAllocation IAllocate(uint32_t byteSize);
        void IFree(ArenaAllocation allocation);

        struct Node
        {
            static constexpr NodeIndex Unused = 0xffffffff;

            uint32_t DataOffset = 0;
            uint32_t DataSize = 0;
            NodeIndex BinListPrev = Unused;
            NodeIndex BinListNext = Unused;
            NodeIndex NeighborPrev = Unused;
            NodeIndex NeighborNext = Unused;
            bool Used = false; // TODO: Merge as bit flag
        };

        uint32_t m_MaxAllocs = 0;
        uint32_t m_FreeStorage = 0;

        uint32_t m_UsedBinsTop = 0;
        uint8_t m_UsedBins[NUM_TOP_BINS] = { 0 };
        NodeIndex m_BinIndices[NUM_LEAF_BINS] = { 0 };

        Node* m_Nodes = nullptr;
        NodeIndex* m_FreeNodes = nullptr;
        uint32_t m_FreeOffset = 0;
    };
}