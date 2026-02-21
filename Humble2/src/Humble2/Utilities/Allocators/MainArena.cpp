#include "MainArena.h"

namespace HBL2
{
    inline uint32_t lzcnt_nonzero(uint32_t v)
    {
#ifdef _MSC_VER
        unsigned long retVal;
        _BitScanReverse(&retVal, v);
        return 31 - retVal;
#else
        return __builtin_clz(v);
#endif
    }

    inline uint32_t tzcnt_nonzero(uint32_t v)
    {
#ifdef _MSC_VER
        unsigned long retVal;
        _BitScanForward(&retVal, v);
        return retVal;
#else
        return __builtin_ctz(v);
#endif
    }

    namespace SmallFloat
    {
        static constexpr uint32_t MANTISSA_BITS = 3;
        static constexpr uint32_t MANTISSA_VALUE = 1 << MANTISSA_BITS;
        static constexpr uint32_t MANTISSA_MASK = MANTISSA_VALUE - 1;

        // Bin sizes follow floating point (exponent + mantissa) distribution (piecewise linear log approx)
        // This ensures that for each size class, the average overhead percentage stays the same
        uint32_t UIntToFloatRoundUp(uint32_t size)
        {
            uint32_t exp = 0;
            uint32_t mantissa = 0;

            if (size < MANTISSA_VALUE)
            {
                // Denorm: 0..(MANTISSA_VALUE-1)
                mantissa = size;
            }
            else
            {
                // Normalized: Hidden high bit always 1. Not stored. Just like float.
                uint32_t leadingZeros = lzcnt_nonzero(size);
                uint32_t highestSetBit = 31 - leadingZeros;

                uint32_t mantissaStartBit = highestSetBit - MANTISSA_BITS;
                exp = mantissaStartBit + 1;
                mantissa = (size >> mantissaStartBit) & MANTISSA_MASK;

                uint32_t lowBitsMask = (1 << mantissaStartBit) - 1;

                // Round up!
                if ((size & lowBitsMask) != 0)
                {
                    mantissa++;
                }
            }

            return (exp << MANTISSA_BITS) + mantissa; // + allows mantissa->exp overflow for round up
        }

        uint32_t UIntToFloatRoundDown(uint32_t size)
        {
            uint32_t exp = 0;
            uint32_t mantissa = 0;

            if (size < MANTISSA_VALUE)
            {
                // Denorm: 0..(MANTISSA_VALUE-1)
                mantissa = size;
            }
            else
            {
                // Normalized: Hidden high bit always 1. Not stored. Just like float.
                uint32_t leadingZeros = lzcnt_nonzero(size);
                uint32_t highestSetBit = 31 - leadingZeros;

                uint32_t mantissaStartBit = highestSetBit - MANTISSA_BITS;
                exp = mantissaStartBit + 1;
                mantissa = (size >> mantissaStartBit) & MANTISSA_MASK;
            }

            return (exp << MANTISSA_BITS) | mantissa;
        }

        uint32_t FloatToUint(uint32_t floatValue)
        {
            uint32_t exponent = floatValue >> MANTISSA_BITS;
            uint32_t mantissa = floatValue & MANTISSA_MASK;
            if (exponent == 0)
            {
                // Denorms
                return mantissa;
            }
            else
            {
                return (mantissa | MANTISSA_VALUE) << (exponent - 1);
            }
        }
    }

    // Utility functions
    uint32_t FindLowestSetBitAfter(uint32_t bitMask, uint32_t startBitIndex)
    {
        uint32_t maskBeforeStartIndex = (1 << startBitIndex) - 1;
        uint32_t maskAfterStartIndex = ~maskBeforeStartIndex;
        uint32_t bitsAfter = bitMask & maskAfterStartIndex;
        if (bitsAfter == 0)
        {
            return ArenaAllocation::NO_SPACE;
        }

        return tzcnt_nonzero(bitsAfter);
    }

    MainArena::MainArena(size_t totalBytes, size_t metaBytes)
    {
        Initialize(totalBytes, metaBytes);
    }

    MainArena::~MainArena()
    {
        // Destroy reservations (placement-new)
        for (auto& r : m_Reservations)
        {
            if (r)
            {
                r->~PoolReservation();
            }
        }
        for (PoolReservation* r : m_FreeReservations)
        {
            if (r)
            {
                r->~PoolReservation();
            }
        }

        m_Reservations.clear();
        m_FreeReservations.clear();

        if (m_Mem)
        {
            std::free(m_Mem);
            m_Mem = nullptr;
        }
    }

    void MainArena::Initialize(size_t totalBytes, size_t metaBytes)
    {
        // Prevent leaks / re-init misuse
        if (m_Mem)
        {
            std::free(m_Mem);
            m_Mem = nullptr;
        }

        m_TotalBytes = totalBytes;

        if (metaBytes == 0 || metaBytes >= totalBytes)
        {
            HBL2_CORE_FATAL("MainArena ERROR: meta bytes must be > 0 and < totalBytes.");
            std::abort();
        }

        m_Mem = static_cast<uint8_t*>(std::malloc(m_TotalBytes));
        if (!m_Mem)
        {
            HBL2_CORE_FATAL("MainArena ERROR: could not allocate {} bytes.", m_TotalBytes);
            std::abort();
        }

        m_MetaBase = m_Mem;
        size_t metaRegionActual = AlignUp(metaBytes, alignof(std::max_align_t));

        if (metaRegionActual >= m_TotalBytes)
        {
            HBL2_CORE_FATAL("MainArena ERROR: meta region consumes entire arena.");
            std::abort();
        }

        m_DataBase = m_Mem + metaRegionActual;
        m_MetaSize = metaRegionActual;
        m_DataSize = m_TotalBytes - metaRegionActual;

        // Ensure DATA heap can have at least one aligned unit
        if (m_DataSize < alignof(std::max_align_t))
        {
            HBL2_CORE_FATAL("MainArena ERROR: DATA region too small ({} bytes).", m_DataSize);
            std::abort();
        }

        // 32-bit heap offsets/size safety
        if (m_DataSize > static_cast<size_t>(0xFFFFFFFFu))
        {
            HBL2_CORE_FATAL("MainArena ERROR: DATA region exceeds 32-bit heap limit ({} bytes).", m_DataSize);
            std::abort();
        }

        m_MetaOffset = 0;

        m_MetaCarved.store(0);
        m_DataInUse.store(0);
        m_DataCarved.store(0);

        m_Reservations.clear();
        m_FreeReservations.clear();
        m_Reservations.reserve(128);

        m_MaxAllocs = 256u * 1024u;
        Reset();
    }

    PoolReservation* MainArena::Reserve(std::string_view name, size_t bytes)
    {
        if (bytes == 0)
        {
            HBL2_CORE_ERROR("MainArena ERROR: cannot reserve 0 bytes (name='{}')", name);
            return nullptr;
        }

        PoolReservation* r = nullptr;

        // Acquire or create the reservation object under META lock.
        {
            std::lock_guard<std::mutex> lk_meta(m_MetaMutex);

            if (!m_FreeReservations.empty())
            {
                r = m_FreeReservations.back();
                m_FreeReservations.pop_back();

                // Reset fields
                CopyName(r->Name, PoolReservation::MaxName, name);
                r->Start = SIZE_MAX;
                r->Size = 0;
                r->Offset = 0;
                r->BackingOffset = ArenaAllocation::NO_SPACE;
                r->BackingMeta = ArenaAllocation::NO_SPACE;

                // Keep capacity, just clear contents
                r->FreeChunks.clear();

#ifdef ARENA_DEBUG
                r->ResetDebugState();
#endif
            }
            else
            {
                void* metaPtr = AllocMetaNoLock(sizeof(PoolReservation), alignof(PoolReservation));
                if (!metaPtr)
                {
                    HBL2_CORE_ERROR("MainArena ERROR: META exhausted creating reservation!");
                    return nullptr;
                }

                r = ::new (metaPtr) PoolReservation(name);
                m_MetaCarved.fetch_add(sizeof(PoolReservation), std::memory_order_relaxed);
            }
        }

        // Allocate backing memory from DATA heap WITHOUT holding META lock.
        size_t alignedByteSize = AlignUp(bytes, alignof(std::max_align_t));
        if (!AllocateReservationBacking(r, alignedByteSize))
        {
            // Return object to free list so we don't leak reservation objects.
            std::lock_guard<std::mutex> lk_meta(m_MetaMutex);
            m_FreeReservations.push_back(r);
            return nullptr;
        }

        // Track live reservations.
        {
            std::lock_guard<std::mutex> lk_meta(m_MetaMutex);
            m_Reservations.emplace_back(r);
        }

        return r;
    }

    void MainArena::TryRelease(PoolReservation* r)
    {
        if (!r)
        {
            return;
        }

        if (r->RefCount.load(std::memory_order_relaxed) > 0)
        {
            return;
        }

#ifdef ARENA_DEBUG
        // If this fires, some arena/chunk is still live.
        const uint32_t live = r->LiveChunks.load(std::memory_order_relaxed);
        HBL2_CORE_ASSERT(live == 0, "Releasing a reservation that still has live chunks");
#endif

        // Invalidate cached chunk structs so they can never be reused after backing memory is freed.
        {
            std::lock_guard<std::mutex> lk(r->FreeChunksMutex);
            for (ArenaChunk* ch : r->FreeChunks)
            {
                if (ch)
                {
                    ch->Data = nullptr;
                    ch->Capacity = 0;
                    ch->Used = 0;
                    ch->Reservation = nullptr;
#ifdef ARENA_DEBUG
                    ch->ReservationGeneration = 0;
#endif
                }
            }
            r->FreeChunks.clear();
        }

        // Free reservation backing block back to DATA heap.
        {
            std::lock_guard<std::mutex> lk(m_DataHeapMutex);

            if (r->BackingMeta != ArenaAllocation::NO_SPACE && r->BackingOffset != ArenaAllocation::NO_SPACE)
            {
#ifdef ARENA_DEBUG
                // Poison reservation memory to catch UAF quickly (before it can be re-used)
                if (r->Start != SIZE_MAX && r->Size != 0)
                {
                    std::memset(m_DataBase + r->Start, 0xDD, r->Size);
                }
#endif

                const size_t sizeFreed = (size_t)m_Nodes[r->BackingMeta].DataSize; // exact heap-used size
                IFree({ r->BackingOffset, r->BackingMeta });

                // Always keep accounting consistent (not debug-only)
                m_DataInUse.fetch_sub(sizeFreed, std::memory_order_relaxed);
            }

            r->BackingOffset = ArenaAllocation::NO_SPACE;
            r->BackingMeta = ArenaAllocation::NO_SPACE;
        }

        // Mark reservation as not-live.
        r->Start = SIZE_MAX;
        r->Size = 0;
        r->Offset = 0;

        // Remove from live-reservations list and put into reusable list.
        {
            std::lock_guard<std::mutex> lk_meta(m_MetaMutex);

            // swap-remove from m_Reservations (avoids O(n) erase shifting)
            for (size_t i = 0; i < m_Reservations.size(); ++i)
            {
                if (m_Reservations[i] == r)
                {
                    m_Reservations[i] = m_Reservations.back();
                    m_Reservations.pop_back();
                    break;
                }
            }

#ifdef ARENA_DEBUG
            r->ResetDebugState();
#endif
            m_FreeReservations.push_back(r);
        }
    }

    void MainArena::Reset()
    {
        m_FreeStorage = 0;
        m_UsedBinsTop = 0;
        m_FreeOffset = m_MaxAllocs;

        for (uint32_t i = 0; i < NUM_TOP_BINS; i++)
        {
            m_UsedBins[i] = 0;
        }

        for (uint32_t i = 0; i < NUM_LEAF_BINS; i++)
        {
            m_BinIndices[i] = Node::Unused;
        }

        m_Nodes = (Node*)AllocMetaNoLock(sizeof(Node) * (m_MaxAllocs + 1), alignof(Node));
        m_FreeNodes = (NodeIndex*)AllocMetaNoLock(sizeof(NodeIndex) * (m_MaxAllocs + 1), alignof(NodeIndex));

        // Freelist is a stack. Nodes in inverse order so that [0] pops first.
        for (uint32_t i = 0; i < m_MaxAllocs + 1; i++)
        {
            m_FreeNodes[i] = m_MaxAllocs - i;
        }

        // Start state: Whole storage as one big node
        // Algorithm will split remainders and push them back as smaller nodes
        InsertNodeIntoBin(m_DataSize, 0);
    }

    uint8_t* MainArena::CarveData(size_t bytes, PoolReservation* r)
    {
        if (!r)
        {
            return nullptr;
        }

        if (bytes == 0)
        {
            HBL2_CORE_ERROR("MainArena ERROR: CarveData(0) is invalid (reservation '{}')", r->Name);
            return nullptr;
        }

#ifdef ARENA_DEBUG
        r->DebugClaimThread();
        HBL2_CORE_ASSERT(r->IsLive(), "CarveData on a non-live reservation");
#endif

        // Reservation bump (NOT thread-safe by requirement)
        const size_t aligned = AlignUp(r->Offset, alignof(std::max_align_t));
        if (aligned + bytes <= r->Size)
        {
            uint8_t* p = m_DataBase + r->Start + aligned;
            r->Offset = aligned + bytes;

            m_DataCarved.fetch_add(bytes, std::memory_order_relaxed);
            return p;
        }

        HBL2_CORE_ERROR("MainArena ERROR: reservation '{}' exhausted (requested {} bytes)", r->Name, bytes);
        return nullptr;
    }

    void* MainArena::AllocMetaNoLock(size_t bytes, size_t alignment)
    {
        size_t aligned = AlignUp(m_MetaOffset, alignment);
        if (aligned + bytes <= m_MetaSize)
        {
            void* p = m_MetaBase + aligned;
            m_MetaOffset = aligned + bytes;
            m_MetaCarved.fetch_add(bytes, std::memory_order_relaxed);
            return p;
        }
        return nullptr;
    }

    void* MainArena::AllocMeta(size_t bytes, size_t alignment)
    {
        std::lock_guard<std::mutex> lk(m_MetaMutex);
        return AllocMetaNoLock(bytes, alignment);
    }

    ArenaChunk* MainArena::AllocateChunkStruct(size_t payload_capacity, PoolReservation* reservation)
    {
        if (!reservation)
        {
            return nullptr;
        }

        if (payload_capacity == 0)
        {
            HBL2_CORE_ERROR("MainArena ERROR: AllocateChunkStruct with 0 payload capacity");
            return nullptr;
        }

#ifdef ARENA_DEBUG
        reservation->DebugClaimThread();
        HBL2_CORE_ASSERT(reservation->IsLive(), "AllocateChunkStruct on a non-live reservation");
#endif

        {
            std::lock_guard<std::mutex> lkRes(reservation->FreeChunksMutex);
            for (size_t i = 0; i < reservation->FreeChunks.size(); ++i)
            {
                ArenaChunk* candidate = reservation->FreeChunks[i];
                if (candidate && candidate->Data && candidate->Capacity >= payload_capacity)
                {
                    reservation->FreeChunks[i] = reservation->FreeChunks.back();
                    reservation->FreeChunks.pop_back();

                    candidate->Used = 0;
                    candidate->Reservation = reservation;
#ifdef ARENA_DEBUG
                    candidate->ReservationGeneration = reservation->Generation;
                    reservation->LiveChunks.fetch_add(1, std::memory_order_relaxed);
#endif
                    return candidate;
                }
            }
        }

        void* structMem = AllocMeta(sizeof(ArenaChunk), alignof(ArenaChunk));
        if (!structMem)
        {
            HBL2_CORE_ERROR("MainArena ERROR: META exhausted while allocating ArenaChunk");
            return nullptr;
        }

        uint8_t* payload = CarveData(payload_capacity, reservation);
        if (!payload)
        {
            return nullptr;
        }

        ArenaChunk* ch = ::new (structMem) ArenaChunk(payload, payload_capacity, reservation);
#ifdef ARENA_DEBUG
        reservation->LiveChunks.fetch_add(1, std::memory_order_relaxed);
#endif
        return ch;
    }

    void MainArena::FreeChunkStruct(ArenaChunk* ch)
    {
        if (!ch)
        {
            return;
        }

#ifdef ARENA_DEBUG
        ch->DebugValidateLive();
#endif

        ch->Used = 0;

        if (ch->Reservation)
        {
#ifdef ARENA_DEBUG
            ch->Reservation->LiveChunks.fetch_sub(1, std::memory_order_relaxed);
#endif
            std::lock_guard<std::mutex> lkRes(ch->Reservation->FreeChunksMutex);
            ch->Reservation->FreeChunks.push_back(ch);
        }
    }
    bool MainArena::AllocateReservationBacking(PoolReservation* r, size_t bytes)
    {
        ArenaAllocation alloc = IAllocate(bytes);

        if (alloc.offset == ArenaAllocation::NO_SPACE && alloc.metadata == ArenaAllocation::NO_SPACE)
        {
            HBL2_CORE_ERROR("MainArena ERROR: DATA heap cannot satisfy reservation ({} bytes)", bytes);
            return false;
        }

        r->Start = (size_t)alloc.offset;
        r->Size = bytes;
        r->Offset = 0;
        r->BackingOffset = alloc.offset;
        r->BackingMeta = alloc.metadata;

        m_DataCarved.fetch_add(bytes, std::memory_order_relaxed);
        m_DataInUse.fetch_add((size_t)m_Nodes[alloc.metadata].DataSize, std::memory_order_relaxed);

        return true;
    }
    uint32_t MainArena::InsertNodeIntoBin(uint32_t size, uint32_t DataOffset)
    {
        // Round down to bin index to ensure that bin >= alloc
        uint32_t binIndex = SmallFloat::UIntToFloatRoundDown(size);

        uint32_t topBinIndex = binIndex >> TOP_BINS_INDEX_SHIFT;
        uint32_t leafBinIndex = binIndex & LEAF_BINS_INDEX_MASK;

        // Bin was empty before?
        if (m_BinIndices[binIndex] == Node::Unused)
        {
            // Set bin mask bits
            m_UsedBins[topBinIndex] |= 1 << leafBinIndex;
            m_UsedBinsTop |= 1 << topBinIndex;
        }

        // Take a freelist node and insert on top of the bin linked list (next = old top)
        uint32_t topNodeIndex = m_BinIndices[binIndex];
        uint32_t nodeIndex = m_FreeNodes[m_FreeOffset--];
#ifdef DEBUG_VERBOSE
        printf("Getting node %u from freelist[%u]\n", nodeIndex, m_FreeOffset + 1);
#endif
        m_Nodes[nodeIndex] = { .DataOffset = DataOffset, .DataSize = size, .BinListNext = topNodeIndex };
        if (topNodeIndex != Node::Unused)
        {
            m_Nodes[topNodeIndex].BinListPrev = nodeIndex;
        }

        m_BinIndices[binIndex] = nodeIndex;

        m_FreeStorage += size;
#ifdef DEBUG_VERBOSE
        printf("Free storage: %u (+%u) (insertNodeIntoBin)\n", m_FreeStorage, size);
#endif

        return nodeIndex;
    }
    void MainArena::RemoveNodeFromBin(uint32_t nodeIndex)
    {
        Node& node = m_Nodes[nodeIndex];

        if (node.BinListPrev != Node::Unused)
        {
            // Easy case: We have previous node. Just remove this node from the middle of the list.
            m_Nodes[node.BinListPrev].BinListNext = node.BinListNext;
            if (node.BinListNext != Node::Unused)
            {
                m_Nodes[node.BinListNext].BinListPrev = node.BinListPrev;
            }
        }
        else
        {
            // Hard case: We are the first node in a bin. Find the bin.

            // Round down to bin index to ensure that bin >= alloc
            uint32_t binIndex = SmallFloat::UIntToFloatRoundDown(node.DataSize);

            uint32_t topBinIndex = binIndex >> TOP_BINS_INDEX_SHIFT;
            uint32_t leafBinIndex = binIndex & LEAF_BINS_INDEX_MASK;

            m_BinIndices[binIndex] = node.BinListNext;
            if (node.BinListNext != Node::Unused)
            {
                m_Nodes[node.BinListNext].BinListPrev = Node::Unused;
            }

            // Bin empty?
            if (m_BinIndices[binIndex] == Node::Unused)
            {
                // Remove a leaf bin mask bit
                m_UsedBins[topBinIndex] &= ~(1 << leafBinIndex);

                // All leaf bins empty?
                if (m_UsedBins[topBinIndex] == 0)
                {
                    // Remove a top bin mask bit
                    m_UsedBinsTop &= ~(1 << topBinIndex);
                }
            }
        }
            // Insert the node to freelist
#ifdef DEBUG_VERBOSE
        printf("Putting node %u into freelist[%u] (removeNodeFromBin)\n", nodeIndex, m_FreeOffset + 1);
#endif
        m_FreeNodes[++m_FreeOffset] = nodeIndex;

        m_FreeStorage -= node.DataSize;
#ifdef DEBUG_VERBOSE
        printf("Free storage: %u (-%u) (removeNodeFromBin)\n", m_FreeStorage, node.DataSize);
#endif
    }
    ArenaAllocation MainArena::IAllocate(uint32_t byteSize)
    {
        // Out of allocations?
        if (m_FreeOffset == ArenaAllocation::NO_SPACE)
        {
            return { .offset = ArenaAllocation::NO_SPACE, .metadata = ArenaAllocation::NO_SPACE };
        }

        // Round up to bin index to ensure that alloc >= bin
        // Gives us min bin index that fits the size
        uint32_t minBinIndex = SmallFloat::UIntToFloatRoundUp(byteSize);

        uint32_t minTopBinIndex = minBinIndex >> TOP_BINS_INDEX_SHIFT;
        uint32_t minLeafBinIndex = minBinIndex & LEAF_BINS_INDEX_MASK;

        uint32_t topBinIndex = minTopBinIndex;
        uint32_t leafBinIndex = ArenaAllocation::NO_SPACE;

        std::lock_guard<std::mutex> lk(m_DataHeapMutex);

        // If top bin exists, scan its leaf bin. This can fail (NO_SPACE).
        if (m_UsedBinsTop & (1 << topBinIndex))
        {
            leafBinIndex = FindLowestSetBitAfter(m_UsedBins[topBinIndex], minLeafBinIndex);
        }

        // If we didn't find space in top bin, we search top bin from +1
        if (leafBinIndex == ArenaAllocation::NO_SPACE)
        {
            topBinIndex = FindLowestSetBitAfter(m_UsedBinsTop, minTopBinIndex + 1);

            // Out of space?
            if (topBinIndex == ArenaAllocation::NO_SPACE)
            {
                return { .offset = ArenaAllocation::NO_SPACE, .metadata = ArenaAllocation::NO_SPACE };
            }

            // All leaf bins here fit the alloc, since the top bin was rounded up. Start leaf search from bit 0.
            // NOTE: This search can't fail since at least one leaf bit was set because the top bit was set.
            leafBinIndex = tzcnt_nonzero(m_UsedBins[topBinIndex]);
        }

        uint32_t binIndex = (topBinIndex << TOP_BINS_INDEX_SHIFT) | leafBinIndex;

        // Pop the top node of the bin. Bin top = node.next.
        uint32_t nodeIndex = m_BinIndices[binIndex];
        Node& node = m_Nodes[nodeIndex];
        uint32_t nodeTotalSize = node.DataSize;
        node.DataSize = byteSize;
        node.Used = true;
        m_BinIndices[binIndex] = node.BinListNext;
        if (node.BinListNext != Node::Unused)
        {
            m_Nodes[node.BinListNext].BinListPrev = Node::Unused;
        }

        m_FreeStorage -= nodeTotalSize;
#ifdef DEBUG_VERBOSE
        printf("Free storage: %u (-%u) (allocate)\n", m_FreeStorage, NodeTotalSize);
#endif

        // Bin empty?
        if (m_BinIndices[binIndex] == Node::Unused)
        {
            // Remove a leaf bin mask bit
            m_UsedBins[topBinIndex] &= ~(1 << leafBinIndex);

            // All leaf bins empty?
            if (m_UsedBins[topBinIndex] == 0)
            {
                // Remove a top bin mask bit
                m_UsedBinsTop &= ~(1 << topBinIndex);
            }
        }

        // Push back reminder N elements to a lower bin
        uint32_t reminderSize = nodeTotalSize - byteSize;
        if (reminderSize > 0)
        {
            uint32_t newNodeIndex = InsertNodeIntoBin(reminderSize, node.DataOffset + byteSize);

            // Link nodes next to each other so that we can merge them later if both are free
            // And update the old next neighbor to point to the new node (in middle)
            if (node.NeighborNext != Node::Unused)
            {
                m_Nodes[node.NeighborNext].NeighborPrev = newNodeIndex;
            }

            m_Nodes[newNodeIndex].NeighborPrev = nodeIndex;
            m_Nodes[newNodeIndex].NeighborNext = node.NeighborNext;
            node.NeighborNext = newNodeIndex;
        }

        return { .offset = node.DataOffset, .metadata = nodeIndex };
    }
    void MainArena::IFree(ArenaAllocation allocation)
    {
        assert(allocation.metadata != ArenaAllocation::NO_SPACE);
        if (!m_Nodes)
        {
            return;
        }

        uint32_t nodeIndex = allocation.metadata;
        Node& node = m_Nodes[nodeIndex];

        // Double delete check
        assert(node.Used == true);

        // Merge with neighbors...
        uint32_t offset = node.DataOffset;
        uint32_t size = node.DataSize;

        if ((node.NeighborPrev != Node::Unused) && (m_Nodes[node.NeighborPrev].Used == false))
        {
            // Previous (contiguous) free node: Change offset to previous node offset. Sum sizes
            Node& prevNode = m_Nodes[node.NeighborPrev];
            offset = prevNode.DataOffset;
            size += prevNode.DataSize;

            // Remove node from the bin linked list and put it in the freelist
            RemoveNodeFromBin(node.NeighborPrev);

            assert(prevNode.NeighborNext == nodeIndex);
            node.NeighborPrev = prevNode.NeighborPrev;
        }

        if ((node.NeighborNext != Node::Unused) && (m_Nodes[node.NeighborNext].Used == false))
        {
            // Next (contiguous) free node: Offset remains the same. Sum sizes.
            Node& nextNode = m_Nodes[node.NeighborNext];
            size += nextNode.DataSize;

            // Remove node from the bin linked list and put it in the freelist
            RemoveNodeFromBin(node.NeighborNext);

            assert(nextNode.NeighborPrev == nodeIndex);
            node.NeighborNext = nextNode.NeighborNext;
        }

        uint32_t neighborNext = node.NeighborNext;
        uint32_t neighborPrev = node.NeighborPrev;

        // Insert the removed node to freelist
#ifdef DEBUG_VERBOSE
        printf("Putting node %u into freelist[%u] (free)\n", nodeIndex, m_FreeOffset + 1);
#endif
        m_FreeNodes[++m_FreeOffset] = nodeIndex;

        // Insert the (combined) free node to bin
        uint32_t combinedNodeIndex = InsertNodeIntoBin(size, offset);

        // Connect neighbors with the new combined node
        if (neighborNext != Node::Unused)
        {
            m_Nodes[combinedNodeIndex].NeighborNext = neighborNext;
            m_Nodes[neighborNext].NeighborPrev = combinedNodeIndex;
        }
        if (neighborPrev != Node::Unused)
        {
            m_Nodes[combinedNodeIndex].NeighborPrev = neighborPrev;
            m_Nodes[neighborPrev].NeighborNext = combinedNodeIndex;
        }
    }

    StorageReport MainArena::GetStorageReport() const
    {
        uint32_t largestFreeRegion = 0;
        uint32_t freeStorage = 0;

        // Out of allocations? -> Zero free space
        if (m_FreeOffset > 0)
        {
            freeStorage = m_FreeStorage;
            if (m_UsedBinsTop)
            {
                uint32_t topBinIndex = 31 - lzcnt_nonzero(m_UsedBinsTop);
                uint32_t leafBinIndex = 31 - lzcnt_nonzero(m_UsedBins[topBinIndex]);
                largestFreeRegion = SmallFloat::FloatToUint((topBinIndex << TOP_BINS_INDEX_SHIFT) | leafBinIndex);
                assert(freeStorage >= largestFreeRegion);
            }
        }

        return { .totalFreeSpace = freeStorage, .largestFreeRegion = largestFreeRegion };
    }

    StorageReportFull MainArena::GetStorageReportFull() const
    {
        StorageReportFull report;
        for (uint32_t i = 0; i < NUM_LEAF_BINS; i++)
        {
            uint32_t count = 0;
            uint32_t nodeIndex = m_BinIndices[i];
            while (nodeIndex != Node::Unused)
            {
                nodeIndex = m_Nodes[nodeIndex].BinListNext;
                count++;
            }
            report.freeRegions[i] = { .size = SmallFloat::FloatToUint(i), .count = count };
        }
        return report;
    }
}
