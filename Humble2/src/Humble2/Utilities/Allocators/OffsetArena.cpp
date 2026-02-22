#include "OffsetArena.h"

namespace HBL2
{
	OffsetArena::~OffsetArena()
    {
        if (!m_Destructed)
        {
            Destroy();
        }
    }

    void OffsetArena::Initialize(MainArena* arena, uint32_t heapBytes, PoolReservation* reservation, uint32_t maxAllocs)
    {
        HBL2_CORE_ASSERT(arena, "OffsetArena: arena null");
        HBL2_CORE_ASSERT(reservation, "OffsetArena: reservation null");

        m_MainArena = arena;
        m_Reservation = reservation;
        m_Reservation->RefCount.fetch_add(1, std::memory_order_relaxed);

        heapBytes = std::max<uint32_t>(heapBytes, 4096u);
        HBL2_CORE_ASSERT(heapBytes < 0xFFFFFFFFu, "OffsetArena: heap must be < 4GB");

        // Allocate metadata from META region.
        m_MaxAllocs = std::max<uint32_t>(maxAllocs, 1024u);

        void* nodesMem = m_MainArena->AllocMeta(sizeof(Node) * m_MaxAllocs, alignof(Node));
        void* freeMem = m_MainArena->AllocMeta(sizeof(uint32_t) * m_MaxAllocs, alignof(uint32_t));

        if (!nodesMem || !freeMem)
        {
            throw std::bad_alloc{};
        }

        m_Nodes = static_cast<Node*>(nodesMem);
        m_FreeNodes = static_cast<uint32_t*>(freeMem);

        uint8_t* base = nullptr;
        uint32_t size = heapBytes;

        uint8_t* sliceBase = m_MainArena->DataBase() + reservation->Start;
        uint8_t* sliceEnd = sliceBase + reservation->Size;

        uintptr_t p = reinterpret_cast<uintptr_t>(sliceBase);
        uintptr_t aligned = OA_AlignUpPtr(p, ALIGNMENT);
        base = reinterpret_cast<uint8_t*>(aligned);

        if (base >= sliceEnd)
        {
            throw std::bad_alloc{};
        }

        size_t avail = (size_t)(sliceEnd - base);
        size = (uint32_t)std::min<size_t>(size, avail);

        size = (uint32_t)OA_AlignDown(size, ALIGNMENT);
        HBL2_CORE_ASSERT(size >= 1024u, "OffsetArena: heap too small after alignment");

        m_HeapBase = base;
        m_HeapSize = size;

        Reset();
    }

    void OffsetArena::Destroy()
    {
        if (m_Reservation)
        {
            m_Reservation->RefCount.fetch_sub(1, std::memory_order_relaxed);
        }

        if (m_MainArena)
        {
            m_MainArena->TryRelease(m_Reservation);
        }

        m_Destructed = true;
    }

    void OffsetArena::Reset()
    {
        m_FreeOffset = 0;
        for (uint32_t i = 0; i < m_MaxAllocs; ++i)
        {
            m_FreeNodes[i] = i;
            m_Nodes[i] = Node{};
        }

        std::memset(m_BinIndices, 0xFF, sizeof(m_BinIndices));
        m_UsedBinsTop = 0;
        std::memset(m_UsedBinsLeaf, 0, sizeof(m_UsedBinsLeaf));

        const uint32_t root = PopFreeNode();
        Node& n = m_Nodes[root];
        n.offset = 0;
        n.size = m_HeapSize;
        n.used = false;

        n.neighborPrev = INVALID;
        n.neighborNext = INVALID;
        n.binPrev = INVALID;
        n.binNext = INVALID;

        m_Head = root;
        m_Tail = root;

        InsertNodeIntoBin(root);
        m_TotalFree = m_HeapSize;
    }

    void* OffsetArena::Alloc(size_t bytes, size_t alignment)
    {
        if (bytes == 0)
        {
            return nullptr;
        }

        alignment = std::max<size_t>(alignment, ALIGNMENT);
        if ((alignment & (alignment - 1)) != 0)
        {
            return nullptr;
        }

        if (alignment > MAX_ALIGNMENT)
        {
            return nullptr;
        }

        // We store a header immediately before the returned pointer:
        // it carries the Allocation handle (offset + metadata).
        const size_t headerSize = sizeof(PointerHeader);

        // We request extra space to place header and satisfy alignment.
        const size_t request = bytes + headerSize + (alignment - 1);

        Allocation a = AllocateInternal((uint32_t)request);
        if (!a.IsValid())
        {
            return nullptr;
        }

        uint8_t* blockBase = m_HeapBase + a.offset;

        // place header so that user pointer after header is aligned
        uintptr_t raw = reinterpret_cast<uintptr_t>(blockBase) + headerSize;
        uintptr_t aligned = OA_AlignUpPtr(raw, alignment);

        PointerHeader* h = reinterpret_cast<PointerHeader*>(aligned - headerSize);
        h->magic = POINTER_MAGIC;
        h->alloc = a;

        return reinterpret_cast<void*>(aligned);
    }

    void OffsetArena::Free(void* p)
    {
        if (!p)
        {
            return;
        }

        uint8_t* user = reinterpret_cast<uint8_t*>(p);
        PointerHeader* h = reinterpret_cast<PointerHeader*>(user - sizeof(PointerHeader));

        // Defensive: ignore non-owned pointers.
        if (h->magic != POINTER_MAGIC)
        {
            return;
        }

        FreeInternal(h->alloc);

        // Poison header to catch double frees early.
        h->magic = 0;
        h->alloc.offset = INVALID;
        h->alloc.metadata = INVALID;
    }

    OffsetArena::StorageReport OffsetArena::GetStorageReport() const
    {
        StorageReport r{};
        r.totalFree = m_TotalFree;
        r.largestFree = FindLargestFree();
        return r;
    }

    bool OffsetArena::Validate() const
    {
        // Validate neighbor chain contiguity and size sums.
        uint32_t idx = m_Head;
        uint32_t expected = 0;
        uint32_t freeSum = 0;

        while (idx != INVALID)
        {
            const Node& n = m_Nodes[idx];
            if (n.offset != expected) return false;
            if (n.size == 0) return false;
            if ((n.offset % ALIGNMENT) != 0) return false;
            if ((n.size % ALIGNMENT) != 0) return false;

            expected += n.size;
            if (!n.used) freeSum += n.size;

            idx = n.neighborNext;
        }

        if (expected != m_HeapSize) return false;
        if (freeSum != m_TotalFree) return false;

        return true;
    }

    uint32_t OffsetArena::PopFreeNode()
    {
        HBL2_CORE_ASSERT(m_FreeOffset < m_MaxAllocs, "OffsetArena: metadata exhausted");
        return m_FreeNodes[m_FreeOffset++];
    }
    void OffsetArena::PushFreeNode(uint32_t idx)
    {
        HBL2_CORE_ASSERT(idx < m_MaxAllocs, "idx OOR");
        HBL2_CORE_ASSERT(m_FreeOffset > 0, "free underflow");
        m_Nodes[idx] = Node{};
        m_FreeNodes[--m_FreeOffset] = idx;
    }
    void OffsetArena::BitmapSet(uint32_t bin)
    {
        const uint32_t top = bin / BINS_PER_LEAF;
        const uint32_t leaf = bin % BINS_PER_LEAF;
        m_UsedBinsLeaf[top] |= (uint8_t)(1u << leaf);
        m_UsedBinsTop |= (1u << top);
    }
    void OffsetArena::BitmapClearIfEmpty(uint32_t bin)
    {
        const uint32_t top = bin / BINS_PER_LEAF;
        const uint32_t leaf = bin % BINS_PER_LEAF;
        if (m_BinIndices[bin] == INVALID)
        {
            m_UsedBinsLeaf[top] &= (uint8_t)~(1u << leaf);
            if (m_UsedBinsLeaf[top] == 0)
            {
                m_UsedBinsTop &= ~(1u << top);
            }
        }
    }
    void OffsetArena::InsertNodeIntoBin(uint32_t idx)
    {
        Node& n = m_Nodes[idx];
        HBL2_CORE_ASSERT(!n.used, "insert free only");

        uint32_t top, leaf;
        SizeToBin(n.size, top, leaf);
        top = std::min<uint32_t>(top, NUM_TOP_BINS - 1);
        leaf = std::min<uint32_t>(leaf, BINS_PER_LEAF - 1);

        const uint32_t bin = TopLeafToBinIndex(top, leaf);

        n.binPrev = INVALID;
        n.binNext = m_BinIndices[bin];
        if (n.binNext != INVALID)
        {
            m_Nodes[n.binNext].binPrev = idx;
        }

        m_BinIndices[bin] = idx;

        BitmapSet(bin);
    }
    void OffsetArena::RemoveNodeFromBin(uint32_t idx)
    {
        Node& n = m_Nodes[idx];
        HBL2_CORE_ASSERT(!n.used, "remove free only");

        uint32_t top, leaf;
        SizeToBin(n.size, top, leaf);
        top = std::min<uint32_t>(top, NUM_TOP_BINS - 1);
        leaf = std::min<uint32_t>(leaf, BINS_PER_LEAF - 1);

        const uint32_t bin = TopLeafToBinIndex(top, leaf);

        if (n.binPrev != INVALID)
        {
            m_Nodes[n.binPrev].binNext = n.binNext;
        }

        if (n.binNext != INVALID)
        {
            m_Nodes[n.binNext].binPrev = n.binPrev;
        }

        if (m_BinIndices[bin] == idx)
        {
            m_BinIndices[bin] = n.binNext;
        }

        n.binPrev = n.binNext = INVALID;
        BitmapClearIfEmpty(bin);
    }
    uint32_t OffsetArena::FindBinForSizeRoundUp(uint32_t size) const
    {
        const uint32_t f = SmallFloat::UIntToFloatRoundUp(size);
        uint32_t top = f >> SmallFloat::MANTISSA_BITS;
        uint32_t leaf = f & SmallFloat::MANTISSA_MASK;

        top = std::min<uint32_t>(top, NUM_TOP_BINS - 1);
        leaf = std::min<uint32_t>(leaf, BINS_PER_LEAF - 1);

        return TopLeafToBinIndex(top, leaf);
    }
    uint32_t OffsetArena::FindFreeNodeForSize(uint32_t needed)
    {
        const uint32_t startBin = FindBinForSizeRoundUp(needed);
        uint32_t top = startBin / BINS_PER_LEAF;
        uint32_t leaf = startBin % BINS_PER_LEAF;

        // same top
        {
            uint8_t leafMask = (uint8_t)(m_UsedBinsLeaf[top] & (uint8_t)(0xFFu << leaf));
            if (leafMask)
            {
                uint32_t l = (uint32_t)tzcnt_nonzero((uint32_t)leafMask);
                uint32_t bin = top * BINS_PER_LEAF + l;
                return m_BinIndices[bin];
            }
        }

        // next tops
        uint32_t topMask = m_UsedBinsTop & (~0u << (top + 1));
        if (!topMask)
        {
            return INVALID;
        }

        uint32_t nextTop = (uint32_t)tzcnt_nonzero(topMask);
        uint8_t leafMask = m_UsedBinsLeaf[nextTop];
        if (!leafMask)
        {
            return INVALID;
        }

        uint32_t l = (uint32_t)tzcnt_nonzero((uint32_t)leafMask);
        uint32_t bin = nextTop * BINS_PER_LEAF + l;
        return m_BinIndices[bin];
    }
    void OffsetArena::LinkInsertAfter(uint32_t existing, uint32_t inserted)
    {
        Node& ex = m_Nodes[existing];
        Node& in = m_Nodes[inserted];

        uint32_t next = ex.neighborNext;
        in.neighborPrev = existing;
        in.neighborNext = next;
        ex.neighborNext = inserted;

        if (next != INVALID)
        {
            m_Nodes[next].neighborPrev = inserted;
        }
        else
        {
            m_Tail = inserted;
        }
    }
    void OffsetArena::LinkInsertBefore(uint32_t existing, uint32_t inserted)
    {
        Node& ex = m_Nodes[existing];
        Node& in = m_Nodes[inserted];

        uint32_t prev = ex.neighborPrev;
        in.neighborPrev = prev;
        in.neighborNext = existing;
        ex.neighborPrev = inserted;

        if (prev != INVALID)
        {
            m_Nodes[prev].neighborNext = inserted;
        }
        else
        {
            m_Head = inserted;
        }
    }
    void OffsetArena::UnlinkNode(uint32_t idx)
    {
        Node& n = m_Nodes[idx];
        uint32_t p = n.neighborPrev;
        uint32_t q = n.neighborNext;

        if (p != INVALID)
        {
            m_Nodes[p].neighborNext = q;
        }
        else
        {
            m_Head = q;
        }

        if (q != INVALID)
        {
            m_Nodes[q].neighborPrev = p;
        }
        else
        {
            m_Tail = p;
        }

        n.neighborPrev = n.neighborNext = INVALID;
    }
    uint32_t OffsetArena::FindLargestFree() const
    {
        uint32_t largest = 0;
        uint32_t idx = m_Head;
        while (idx != INVALID)
        {
            const Node& n = m_Nodes[idx];
            if (!n.used)
            {
                largest = std::max(largest, n.size);
            }

            idx = n.neighborNext;
        }

        return largest;
    }
    OffsetArena::Allocation OffsetArena::AllocateInternal(uint32_t sizeBytes)
    {
        // sizeBytes must be aligned to ALIGNMENT in original code
        const uint32_t size = (uint32_t)OA_AlignUp(sizeBytes, ALIGNMENT);

        uint32_t freeNode = FindFreeNodeForSize(size);
        if (freeNode == INVALID)
        {
            return {};
        }

        RemoveNodeFromBin(freeNode);

        Node& n = m_Nodes[freeNode];
        HBL2_CORE_ASSERT(!n.used, "expected free");

        // Split if leftover is big enough (sebbbi uses MIN_REGION_SIZE = 16)
        const uint32_t remaining = n.size - size;
        if (remaining >= 16u)
        {
            const uint32_t splitNode = PopFreeNode();
            Node& s = m_Nodes[splitNode];
            s.used = false;
            s.offset = n.offset + size;
            s.size = remaining;

            // Fix neighbor links: n -> s -> oldNext
            LinkInsertAfter(freeNode, splitNode);
            InsertNodeIntoBin(splitNode);

            n.size = size;
        }

        n.used = true;
        n.binPrev = n.binNext = INVALID;

        m_TotalFree -= n.size;
        return Allocation{ n.offset, freeNode };
    }
    void OffsetArena::FreeInternal(OffsetArena::Allocation a)
    {
        if (!a.IsValid())
        {
            return;
        }

        HBL2_CORE_ASSERT(a.metadata < m_MaxAllocs, "metadata OOR");

        Node& n = m_Nodes[a.metadata];
        HBL2_CORE_ASSERT(n.used, "double free / corruption");
        HBL2_CORE_ASSERT(n.offset == a.offset, "allocation handle mismatch");

        n.used = false;

        // Coalesce with prev
        if (n.neighborPrev != INVALID && !m_Nodes[n.neighborPrev].used)
        {
            uint32_t prevIdx = n.neighborPrev;
            Node& p = m_Nodes[prevIdx];

            RemoveNodeFromBin(prevIdx);

            p.size += n.size;
            UnlinkNode(a.metadata);
            PushFreeNode(a.metadata);

            // Continue with merged node
            a.metadata = prevIdx;
        }

        // Coalesce with next
        uint32_t nextIdx = m_Nodes[a.metadata].neighborNext;
        if (nextIdx != INVALID && !m_Nodes[nextIdx].used)
        {
            RemoveNodeFromBin(nextIdx);

            m_Nodes[a.metadata].size += m_Nodes[nextIdx].size;
            UnlinkNode(nextIdx);
            PushFreeNode(nextIdx);
        }

        InsertNodeIntoBin(a.metadata);

        // Recompute total free conservatively (simple + safe)
        uint32_t total = 0;
        uint32_t idx = m_Head;
        while (idx != INVALID)
        {
            if (!m_Nodes[idx].used)
            {
                total += m_Nodes[idx].size;
            }

            idx = m_Nodes[idx].neighborNext;
        }
        m_TotalFree = total;
    }
}
