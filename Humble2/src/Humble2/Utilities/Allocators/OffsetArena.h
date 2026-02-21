#pragma once

#include "Base.h"
#include "MainArena.h"

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <algorithm>

#if defined(_MSC_VER)
# include <intrin.h>
#endif

namespace HBL2
{
    static inline size_t OA_AlignUp(size_t v, size_t a) { return (v + (a - 1)) & ~(a - 1); }
    static inline uintptr_t OA_AlignUpPtr(uintptr_t v, size_t a) { return (v + (a - 1)) & ~(uintptr_t(a - 1)); }
    static inline size_t OA_AlignDown(size_t v, size_t a) { return v & ~(a - 1); }

    /**
     * @brief ...
     *
     * @note Inspired by Sebastian Aaltonens' OffsetAllocator and adjusted to use pointers API (https://github.com/sebbbi/OffsetAllocator).
     */
    class HBL2_API OffsetArena
    {
    public:
        static constexpr uint32_t INVALID = 0xFFFFFFFFu;

        struct Allocation
        {
            uint32_t offset = INVALID;
            uint32_t metadata = INVALID;
            bool IsValid() const { return offset != INVALID; }
        };

        struct StorageReport
        {
            uint32_t totalFree = 0;
            uint32_t largestFree = 0;
        };

        OffsetArena() = default;
        ~OffsetArena();

        void Initialize(MainArena* arena, uint32_t heapBytes, PoolReservation* reservation, uint32_t maxAllocs = DEFAULT_MAX_ALLOCS);

        void Destroy();

        void Reset();

        void* Alloc(size_t bytes, size_t alignment = alignof(std::max_align_t));

        void Free(void* p);

        StorageReport GetStorageReport() const;

        bool Validate() const;

        inline uint8_t* HeapBase() const { return m_HeapBase; }
        inline uint32_t HeapSize() const { return m_HeapSize; }

    private:
        struct SmallFloat
        {
            static constexpr uint32_t MANTISSA_BITS = 3;
            static constexpr uint32_t MANTISSA_VALUE = 1u << MANTISSA_BITS;
            static constexpr uint32_t MANTISSA_MASK = MANTISSA_VALUE - 1;

            static inline uint32_t UIntToFloatRoundUp(uint32_t size)
            {
                // Rounds up to the next representable "float" bin.
                // Works for size > 0.
                uint32_t exp = 0;
                uint32_t mant = 0;

                if (size < MANTISSA_VALUE)
                {
                    mant = size;
                }
                else
                {
                    // Find exponent: floor(log2(size))
                    uint32_t msb = 31u - (uint32_t)lzcnt_nonzero(size);
                    exp = msb - MANTISSA_BITS;
                    const uint32_t shift = exp;
                    mant = (size >> shift) & MANTISSA_MASK;

                    // If there are lower bits set, round mant up.
                    const uint32_t lowMask = (1u << shift) - 1u;
                    if ((size & lowMask) != 0)
                    {
                        mant++;
                        if (mant >= MANTISSA_VALUE)
                        {
                            mant = 0;
                            exp++;
                        }
                    }
                }

                return (exp << MANTISSA_BITS) | mant;
            }

            static inline uint32_t UIntToFloatRoundDown(uint32_t size)
            {
                // Rounds down to representable.
                uint32_t exp = 0;
                uint32_t mant = 0;

                if (size < MANTISSA_VALUE)
                {
                    mant = size;
                }
                else
                {
                    uint32_t msb = 31u - (uint32_t)lzcnt_nonzero(size);
                    exp = msb - MANTISSA_BITS;
                    mant = (size >> exp) & MANTISSA_MASK;
                }

                return (exp << MANTISSA_BITS) | mant;
            }

            static inline uint32_t FloatToUInt(uint32_t f)
            {
                const uint32_t exp = f >> MANTISSA_BITS;
                const uint32_t mant = f & MANTISSA_MASK;
                return (mant | MANTISSA_VALUE) << exp;
            }
        };

        static constexpr size_t ALIGNMENT = 8;
        static constexpr size_t MAX_ALIGNMENT = 256;        

        static constexpr uint32_t NUM_TOP_BINS = 32;
        static constexpr uint32_t BINS_PER_LEAF = 8;
        static constexpr uint32_t NUM_LEAF_BINS = NUM_TOP_BINS * BINS_PER_LEAF; // 256

        static constexpr uint32_t POINTER_MAGIC = 0xAC1D0FFEu;

        struct Node
        {
            uint32_t offset = 0;
            uint32_t size = 0;
            bool used = false;

            // linked list in bin
            uint32_t binPrev = INVALID;
            uint32_t binNext = INVALID;

            // physical neighbor list
            uint32_t neighborPrev = INVALID;
            uint32_t neighborNext = INVALID;
        };

        struct PointerHeader
        {
            uint32_t magic;
            Allocation alloc;
        };

        MainArena* m_MainArena = nullptr;
        PoolReservation* m_Reservation = nullptr;
        bool m_Destructed = false;

        uint8_t* m_HeapBase = nullptr;
        uint32_t m_HeapSize = 0;

        static constexpr uint32_t DEFAULT_MAX_ALLOCS = 256u * 1024u;

        Node* m_Nodes = nullptr;
        uint32_t* m_FreeNodes = nullptr;
        uint32_t  m_MaxAllocs = 0;
        uint32_t m_FreeOffset = 0;

        uint32_t m_BinIndices[NUM_LEAF_BINS]{};
        uint32_t m_UsedBinsTop = 0;
        uint8_t  m_UsedBinsLeaf[NUM_TOP_BINS]{};

        uint32_t m_Head = INVALID;
        uint32_t m_Tail = INVALID;

        uint32_t m_TotalFree = 0;

        uint32_t PopFreeNode();

        void PushFreeNode(uint32_t idx);

        static inline uint32_t lzcnt_nonzero(uint32_t v)
        {
#ifdef _MSC_VER
            unsigned long retVal;
            _BitScanReverse(&retVal, v);
            return 31 - retVal;
#else
            return __builtin_clz(v);
#endif
        }

        static inline uint32_t tzcnt_nonzero(uint32_t v)
        {
#ifdef _MSC_VER
            unsigned long retVal;
            _BitScanForward(&retVal, v);
            return retVal;
#else
            return __builtin_ctz(v);
#endif
        }

        static inline void SizeToBin(uint32_t size, uint32_t& topBin, uint32_t& leafBin)
        {
            // size must be >= 1
            const uint32_t f = SmallFloat::UIntToFloatRoundDown(size);
            topBin = f >> SmallFloat::MANTISSA_BITS;
            leafBin = f & SmallFloat::MANTISSA_MASK;
        }

        static inline uint32_t TopLeafToBinIndex(uint32_t top, uint32_t leaf)
        {
            return top * BINS_PER_LEAF + leaf;
        }

        void BitmapSet(uint32_t bin);

        void BitmapClearIfEmpty(uint32_t bin);

        void InsertNodeIntoBin(uint32_t idx);

        void RemoveNodeFromBin(uint32_t idx);

        // Find first bin >= requested size.
        uint32_t FindBinForSizeRoundUp(uint32_t size) const;

        uint32_t FindFreeNodeForSize(uint32_t needed);

        // Neighbor list ops
        void LinkInsertAfter(uint32_t existing, uint32_t inserted);

        void LinkInsertBefore(uint32_t existing, uint32_t inserted);

        void UnlinkNode(uint32_t idx);

        uint32_t FindLargestFree() const;

        Allocation AllocateInternal(uint32_t sizeBytes);

        void FreeInternal(Allocation a);
    };
}
