#pragma once

#include "BaseAllocator.h"
#include "Utilities\Log.h"

#include <cstring>
#include <stdint.h>
#include <type_traits>

namespace HBL2
{
    /**
     * @brief A memory allocator that organizes free memory into bins based on size categories
     *        using an exponent-mantissa encoding.
     *
     * This allows efficient allocation by searching for the smallest available block in the
     * appropriate bin using a first-fit strategy. When memory is deallocated, it is returned
     * to the free list within its corresponding bin for reuse.
     *
     * To minimize fragmentation, if an allocated block is not fully used, the remaining space
     * is split and stored in a suitable bin. This ensures that memory utilization remains efficient
     * while keeping allocation and deallocation operations fast.
     *
     * @note This implementation is inspired by the open-source Offset Allocator by Sebastian Aaltonen:
     *       https://github.com/sebbbi/OffsetAllocator
     */
    class BinAllocator final : public BaseAllocator<BinAllocator>
    {
    public:
        struct AllocationInfo
        {
            static constexpr uint32_t INVALID = 0xFFFFFFFF;
            uint32_t nodeIndex = INVALID;
        };

        struct StorageReport
        {
            uint64_t totalFreeSpace;
            uint64_t largestFreeRegion;
        };

    public:
        BinAllocator() = default;

        /**
         * @brief Initializes an offset allocator with a given memory size.
         *
         * @param size The total size of the memory pool in bytes.
         * @param maxAllocs Maximum number of concurrent allocations (default: 128k).
         */
        BinAllocator(uint64_t size, uint32_t maxAllocs = 128 * 1024)
        {
            Initialize(size, maxAllocs);
        }

        /**
         * @brief Allocates a block of memory of the given size.
         *
         * @tparam T The type of object to allocate.
         * @param size The size of the allocation in bytes (default: sizeof(T)).
         * @return A pointer to the allocated memory, or nullptr if out of memory.
         */
        template<typename T>
        T* Allocate(uint64_t size = sizeof(T))
        {
            if (m_FreeOffset == 0)
            {
                HBL2_CORE_ERROR("BinAllocator out of allocations!");
                return nullptr;
            }

            // Ensure minimum alignment for type T
            constexpr uint64_t alignment = alignof(T);
            uint64_t alignedSize = (size + alignment - 1) & ~(alignment - 1);

            uint32_t minBinIndex = UintToFloatRoundUp((uint32_t)alignedSize);

            uint32_t minTopBinIndex = minBinIndex >> TOP_BINS_INDEX_SHIFT;
            uint32_t minLeafBinIndex = minBinIndex & LEAF_BINS_INDEX_MASK;

            uint32_t topBinIndex = minTopBinIndex;
            uint32_t leafBinIndex = AllocationInfo::INVALID;

            if (m_UsedBinsTop & (1 << topBinIndex))
            {
                leafBinIndex = FindLowestSetBitAfter(m_UsedBins[topBinIndex], minLeafBinIndex);
            }

            if (leafBinIndex == AllocationInfo::INVALID)
            {
                topBinIndex = FindLowestSetBitAfter(m_UsedBinsTop, minTopBinIndex + 1);

                if (topBinIndex == AllocationInfo::INVALID)
                {
                    HBL2_CORE_ERROR("BinAllocator out of memory!");
                    return nullptr;
                }

                leafBinIndex = tzcnt_nonzero(m_UsedBins[topBinIndex]);
            }

            uint32_t binIndex = (topBinIndex << TOP_BINS_INDEX_SHIFT) | leafBinIndex;

            uint32_t nodeIndex = m_BinIndices[binIndex];
            Node& node = m_Nodes[nodeIndex];
            uint64_t nodeTotalSize = node.dataSize;
            node.dataSize = alignedSize;
            node.used = true;
            m_BinIndices[binIndex] = node.binListNext;

            if (node.binListNext != Node::UNUSED)
            {
                m_Nodes[node.binListNext].binListPrev = Node::UNUSED;
            }

            m_FreeStorage -= nodeTotalSize;

            if (m_BinIndices[binIndex] == Node::UNUSED)
            {
                m_UsedBins[topBinIndex] &= ~(1 << leafBinIndex);

                if (m_UsedBins[topBinIndex] == 0)
                {
                    m_UsedBinsTop &= ~(1 << topBinIndex);
                }
            }

            uint64_t reminderSize = nodeTotalSize - alignedSize;
            if (reminderSize > 0)
            {
                void* reminderPtr = (char*)node.dataPtr + alignedSize;
                uint32_t newNodeIndex = InsertNodeIntoBin(reminderSize, reminderPtr);

                if (node.neighborNext != Node::UNUSED)
                {
                    m_Nodes[node.neighborNext].neighborPrev = newNodeIndex;
                }

                m_Nodes[newNodeIndex].neighborPrev = nodeIndex;
                m_Nodes[newNodeIndex].neighborNext = node.neighborNext;
                node.neighborNext = newNodeIndex;
            }

            // Apply alignment to the returned pointer
            void* alignedPtr = node.dataPtr;
            if (alignment > 1)
            {
                uintptr_t addr = reinterpret_cast<uintptr_t>(node.dataPtr);
                uintptr_t alignedAddr = (addr + alignment - 1) & ~(alignment - 1);
                alignedPtr = reinterpret_cast<void*>(alignedAddr);
            }

            return static_cast<T*>(alignedPtr);
        }

        /**
         * @brief Deallocates a previously allocated block of memory.
         *
         * @tparam T The type of object to deallocate.
         * @param ptr The pointer to deallocate.
         * @param allocationInfo The allocation info returned during allocation.
         */
        template<typename T>
        void Deallocate(T* ptr)
        {
            if (!ptr || !m_Nodes) return;

            // Find the node that contains this pointer
            uint32_t nodeIndex = FindNodeContainingPointer(ptr);
            if (nodeIndex == Node::UNUSED)
            {
                HBL2_CORE_ERROR("OffsetAllocator: Invalid pointer for deallocation!");
                return;
            }

            Node& node = m_Nodes[nodeIndex];

            if (!node.used)
            {
                HBL2_CORE_ERROR("OffsetAllocator: Double free detected!");
                return;
            }

            void* dataPtr = node.dataPtr;
            uint64_t size = node.dataSize;

            if ((node.neighborPrev != Node::UNUSED) && (m_Nodes[node.neighborPrev].used == false))
            {
                Node& prevNode = m_Nodes[node.neighborPrev];
                dataPtr = prevNode.dataPtr;
                size += prevNode.dataSize;

                RemoveNodeFromBin(node.neighborPrev);

                node.neighborPrev = prevNode.neighborPrev;
            }

            if ((node.neighborNext != Node::UNUSED) && (m_Nodes[node.neighborNext].used == false))
            {
                Node& nextNode = m_Nodes[node.neighborNext];
                size += nextNode.dataSize;

                RemoveNodeFromBin(node.neighborNext);

                node.neighborNext = nextNode.neighborNext;
            }

            uint32_t neighborNext = node.neighborNext;
            uint32_t neighborPrev = node.neighborPrev;

            m_FreeNodes[++m_FreeOffset] = nodeIndex;

            uint32_t combinedNodeIndex = InsertNodeIntoBin(size, dataPtr);

            if (neighborNext != Node::UNUSED)
            {
                m_Nodes[combinedNodeIndex].neighborNext = neighborNext;
                m_Nodes[neighborNext].neighborPrev = combinedNodeIndex;
            }

            if (neighborPrev != Node::UNUSED)
            {
                m_Nodes[combinedNodeIndex].neighborPrev = neighborPrev;
                m_Nodes[neighborPrev].neighborNext = combinedNodeIndex;
            }
        }

        /**
         * @brief Initializes the allocator with a specified memory size.
         *
         * @param sizeInBytes The total size of the memory pool in bytes.
         * @param maxAllocs Maximum number of concurrent allocations.
         */
        virtual void Initialize(size_t sizeInBytes) override
        {
            Initialize(sizeInBytes, 1024 * 1024);
        }

        void Initialize(size_t sizeInBytes, uint32_t maxAllocs)
        {
            m_Size = sizeInBytes;
            m_MaxAllocs = maxAllocs;
            m_FreeStorage = 0;
            m_UsedBinsTop = 0;
            m_FreeOffset = m_MaxAllocs - 1;

            for (uint32_t i = 0; i < NUM_TOP_BINS; i++)
            {
                m_UsedBins[i] = 0;
            }

            for (uint32_t i = 0; i < NUM_LEAF_BINS; i++)
            {
                m_BinIndices[i] = Node::UNUSED;
            }

            if (m_Nodes) delete[] m_Nodes;
            if (m_FreeNodes) delete[] m_FreeNodes;
            if (m_Data) ::operator delete(m_Data);

            m_Data = ::operator new(m_Size);
            std::memset(m_Data, 0, m_Size);

            m_Nodes = new Node[m_MaxAllocs];
            m_FreeNodes = new NodeIndex[m_MaxAllocs];

            for (uint32_t i = 0; i < m_MaxAllocs; i++)
            {
                m_FreeNodes[i] = m_MaxAllocs - i - 1;
            }

            InsertNodeIntoBin(m_Size, m_Data);
        }

        /**
         * @brief Resets the allocator, clearing all allocated memory.
         */
        virtual void Invalidate() override
        {
            if (m_Data)
            {
                std::memset(m_Data, 0, m_Size);

                m_FreeStorage = 0;
                m_UsedBinsTop = 0;
                m_FreeOffset = m_MaxAllocs - 1;

                for (uint32_t i = 0; i < NUM_TOP_BINS; i++)
                {
                    m_UsedBins[i] = 0;
                }

                for (uint32_t i = 0; i < NUM_LEAF_BINS; i++)
                {
                    m_BinIndices[i] = Node::UNUSED;
                }

                for (uint32_t i = 0; i < m_MaxAllocs; i++)
                {
                    m_FreeNodes[i] = m_MaxAllocs - i - 1;
                }

                InsertNodeIntoBin(m_Size, m_Data);
            }
        }

        /**
         * @brief Frees all allocated memory and resets the allocator.
         */
        virtual void Free() override
        {
            if (m_Nodes) delete[] m_Nodes;
            if (m_FreeNodes) delete[] m_FreeNodes;
            if (m_Data) ::operator delete(m_Data);

            m_Nodes = nullptr;
            m_FreeNodes = nullptr;
            m_Data = nullptr;
            m_FreeOffset = 0;
            m_MaxAllocs = 0;
            m_UsedBinsTop = 0;
            m_Size = 0;
            m_FreeStorage = 0;
        }

        /**
         * @brief Gets storage statistics.
         */
        StorageReport GetStorageReport()
        {
            uint64_t largestFreeRegion = 0;
            uint64_t freeStorage = 0;

            if (m_FreeOffset > 0)
            {
                freeStorage = m_FreeStorage;
                if (m_UsedBinsTop)
                {
                    uint32_t topBinIndex = 31 - lzcnt_nonzero(m_UsedBinsTop);
                    uint32_t leafBinIndex = 31 - lzcnt_nonzero(m_UsedBins[topBinIndex]);
                    largestFreeRegion = FloatToUint((topBinIndex << TOP_BINS_INDEX_SHIFT) | leafBinIndex);
                }
            }

            return { .totalFreeSpace = freeStorage, .largestFreeRegion = largestFreeRegion };
        }

        float GetFullPercentage() const
        {
            return ((float)(m_Size - m_FreeStorage) / (float)m_Size) * 100.0f;
        }

    private:
        uint32_t InsertNodeIntoBin(uint64_t size, void* dataPtr)
        {
            uint32_t binIndex = UintToFloatRoundDown((uint32_t)size);

            uint32_t topBinIndex = binIndex >> TOP_BINS_INDEX_SHIFT;
            uint32_t leafBinIndex = binIndex & LEAF_BINS_INDEX_MASK;

            if (m_BinIndices[binIndex] == Node::UNUSED)
            {
                m_UsedBins[topBinIndex] |= 1 << leafBinIndex;
                m_UsedBinsTop |= 1 << topBinIndex;
            }

            uint32_t topNodeIndex = m_BinIndices[binIndex];
            uint32_t nodeIndex = m_FreeNodes[m_FreeOffset--];

            m_Nodes[nodeIndex] = {
                .dataPtr = dataPtr,
                .dataSize = size,
                .binListNext = topNodeIndex
            };

            if (topNodeIndex != Node::UNUSED)
            {
                m_Nodes[topNodeIndex].binListPrev = nodeIndex;
            }

            m_BinIndices[binIndex] = nodeIndex;

            m_FreeStorage += size;

            return nodeIndex;
        }

        void RemoveNodeFromBin(uint32_t nodeIndex)
        {
            Node& node = m_Nodes[nodeIndex];

            if (node.binListPrev != Node::UNUSED)
            {
                m_Nodes[node.binListPrev].binListNext = node.binListNext;

                if (node.binListNext != Node::UNUSED)
                {
                    m_Nodes[node.binListNext].binListPrev = node.binListPrev;
                }
            }
            else
            {
                uint32_t binIndex = UintToFloatRoundDown((uint32_t)node.dataSize);

                uint32_t topBinIndex = binIndex >> TOP_BINS_INDEX_SHIFT;
                uint32_t leafBinIndex = binIndex & LEAF_BINS_INDEX_MASK;

                m_BinIndices[binIndex] = node.binListNext;
                if (node.binListNext != Node::UNUSED)
                {
                    m_Nodes[node.binListNext].binListPrev = Node::UNUSED;
                }

                if (m_BinIndices[binIndex] == Node::UNUSED)
                {
                    m_UsedBins[topBinIndex] &= ~(1 << leafBinIndex);

                    if (m_UsedBins[topBinIndex] == 0)
                    {
                        m_UsedBinsTop &= ~(1 << topBinIndex);
                    }
                }
            }

            m_FreeNodes[++m_FreeOffset] = nodeIndex;
            m_FreeStorage -= node.dataSize;
        }

        uint32_t FindNodeContainingPointer(void* ptr)
        {
            char* searchPtr = static_cast<char*>(ptr);
            char* basePtr = static_cast<char*>(m_Data);

            if (searchPtr < basePtr || searchPtr >= basePtr + m_Size)
            {
                return Node::UNUSED;
            }

            // Linear search through used nodes (could be optimized with spatial data structure)
            for (uint32_t i = 0; i < m_MaxAllocs; i++)
            {
                if (m_Nodes[i].used && m_Nodes[i].dataPtr)
                {
                    char* nodeStart = static_cast<char*>(m_Nodes[i].dataPtr);
                    char* nodeEnd = nodeStart + m_Nodes[i].dataSize;

                    if (searchPtr >= nodeStart && searchPtr < nodeEnd)
                    {
                        return i;
                    }
                }
            }

            return Node::UNUSED;
        }

    private:
        using NodeIndex = uint32_t;

        static constexpr uint32_t NUM_TOP_BINS = 32;
        static constexpr uint32_t BINS_PER_LEAF = 8;
        static constexpr uint32_t TOP_BINS_INDEX_SHIFT = 3;
        static constexpr uint32_t LEAF_BINS_INDEX_MASK = 0x7;
        static constexpr uint32_t NUM_LEAF_BINS = NUM_TOP_BINS * BINS_PER_LEAF;

        // Small float representation for size classes
        static constexpr uint32_t MANTISSA_BITS = 3;
        static constexpr uint32_t MANTISSA_VALUE = 1 << MANTISSA_BITS;
        static constexpr uint32_t MANTISSA_MASK = MANTISSA_VALUE - 1;

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

        uint32_t UintToFloatRoundUp(uint32_t size)
        {
            uint32_t exp = 0;
            uint32_t mantissa = 0;

            if (size < MANTISSA_VALUE)
            {
                mantissa = size;
            }
            else
            {
                uint32_t leadingZeros = lzcnt_nonzero(size);
                uint32_t highestSetBit = 31 - leadingZeros;

                uint32_t mantissaStartBit = highestSetBit - MANTISSA_BITS;
                exp = mantissaStartBit + 1;
                mantissa = (size >> mantissaStartBit) & MANTISSA_MASK;

                uint32_t lowBitsMask = (1 << mantissaStartBit) - 1;

                if ((size & lowBitsMask) != 0)
                {
                    mantissa++;
                }
            }

            return (exp << MANTISSA_BITS) + mantissa;
        }

        uint32_t UintToFloatRoundDown(uint32_t size)
        {
            uint32_t exp = 0;
            uint32_t mantissa = 0;

            if (size < MANTISSA_VALUE)
            {
                mantissa = size;
            }
            else
            {
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
                return mantissa;
            }
            else
            {
                return (mantissa | MANTISSA_VALUE) << (exponent - 1);
            }
        }

        struct Node
        {
            static constexpr NodeIndex UNUSED = 0xFFFFFFFF;

            void* dataPtr = nullptr;
            uint64_t dataSize = 0;
            NodeIndex binListPrev = UNUSED;
            NodeIndex binListNext = UNUSED;
            NodeIndex neighborPrev = UNUSED;
            NodeIndex neighborNext = UNUSED;
            bool used = false;
        };

        uint32_t FindLowestSetBitAfter(uint32_t bitMask, uint32_t startBitIndex)
        {
            uint32_t maskBeforeStartIndex = (1 << startBitIndex) - 1;
            uint32_t maskAfterStartIndex = ~maskBeforeStartIndex;
            uint32_t bitsAfter = bitMask & maskAfterStartIndex;
            if (bitsAfter == 0) return AllocationInfo::INVALID;
            return tzcnt_nonzero(bitsAfter);
        }

    private:
        void* m_Data = nullptr;
        uint64_t m_Size = 0;
        uint32_t m_MaxAllocs = 0;
        uint64_t m_FreeStorage = 0;

        uint32_t m_UsedBinsTop = 0;
        uint8_t m_UsedBins[NUM_TOP_BINS];
        NodeIndex m_BinIndices[NUM_LEAF_BINS];

        Node* m_Nodes = nullptr;
        NodeIndex* m_FreeNodes = nullptr;
        uint32_t m_FreeOffset = 0;
    };
}