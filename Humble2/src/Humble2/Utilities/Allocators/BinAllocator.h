#pragma once

#include "BaseAllocator.h"
#include "Utilities\Log.h"

#include <cstring>
#include <stdint.h>

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
        BinAllocator() = default;

        /**
         * @brief Initializes the allocator with a given memory pool size.
         *
         * @param size The total size of the memory pool in bytes.
         */
        BinAllocator(uint64_t size)
        {
            Initialize(size);
        }

        /**
         * @brief Allocates a block of memory of the specified size.
         *
         * If a suitable free block is found in the bins, it is used. Otherwise, memory is
         * allocated from the main pool.
         *
         * @tparam T The type of object to allocate.
         * @param size The size of the allocation in bytes (default: sizeof(T)).
         * @return A pointer to the allocated memory, or nullptr if out of memory.
         */
        template<typename T>
        T* Allocate(uint64_t size = sizeof(T))
        {
            m_AllocatedBytes += size;

            uint32_t binIndex = FindBinIndex(size);

            for (uint32_t i = binIndex; i < NUM_LEAF_BINS; i++)
            {
                if (m_Bins[i].FreeList)
                {
                    FreeBlock* block = m_Bins[i].FreeList;
                    RemoveFromBin(i, block);

                    if (block->Size > size + sizeof(FreeBlock))
                    {
                        // Split the block
                        void* newBlock = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(block) + size);
                        InsertIntoBin(newBlock, block->Size - size);
                    }

                    return reinterpret_cast<T*>(block);
                }
            }

            // If no free block found, allocate from the current offset
            constexpr uint64_t alignment = alignof(T);
            uint64_t alignedOffset = (m_CurrentOffset + (alignment - 1)) & ~(alignment - 1);

            if (alignedOffset + size > m_Capacity)
            {
                HBL2_CORE_ERROR("BinAllocator out of memory!");
                return nullptr;
            }

            T* ptr = reinterpret_cast<T*>(static_cast<uint8_t*>(m_Data) + alignedOffset);
            m_CurrentOffset = alignedOffset + size;
            return ptr;
        }

        /**
         * @brief Deallocates memory by placing the block back into the appropriate bin.
         *
         * @tparam T The type of object being deallocated.
         * @param object The pointer to the memory block being freed.
         */
        template<typename T>
        void Deallocate(T* object)
        {
            if (!object)
            {
                return;
            }

            uint64_t blockSize = sizeof(T);
            InsertIntoBin(object, blockSize);
            m_AllocatedBytes -= blockSize;
        }

        /**
         * @brief Initializes the allocator with a specified memory size.
         *
         * @param sizeInBytes The total size of the memory pool in bytes.
         */
        virtual void Initialize(size_t sizeInBytes) override
        {
            m_Capacity = sizeInBytes;
            m_CurrentOffset = 0;

            m_Data = ::operator new(m_Capacity);
            std::memset(m_Data, 0, m_Capacity);

            for (uint32_t i = 0; i < NUM_LEAF_BINS; i++)
            {
                m_Bins[i].FreeList = nullptr;
            }

            // Initialize the allocator with a single large free block
            InsertIntoBin(m_Data, m_Capacity);
        }

        /**
         * @brief Resets the allocator, clearing all allocated memory without deallocating the memory pool itself.
         */
        virtual void Invalidate() override
        {
            std::memset(m_Data, 0, m_CurrentOffset);
            m_CurrentOffset = 0;
			m_AllocatedBytes = 0;

            for (uint32_t i = 0; i < NUM_LEAF_BINS; i++)
            {
                m_Bins[i].FreeList = nullptr;
            }

            // Insert the entire memory block as free space
            InsertIntoBin(m_Data, m_Capacity);
        }

        /**
         * @brief Frees all allocated memory and resets the allocator.
         *
         * After calling this, the allocator cannot be used until reinitialized.
         */
        virtual void Free() override
        {
            ::operator delete(m_Data);
			m_Data = nullptr;
            m_Capacity = 0;
            m_CurrentOffset = 0;
			m_AllocatedBytes = 0;
        }

		float GetFullPercentage()
		{
			return ((float)m_AllocatedBytes / (float)m_Capacity) * 100.f;
		}

    private:
        /**
         * @brief Represents a free memory block in the allocator.
         */
        struct FreeBlock
        {
            FreeBlock* Next;
            uint64_t Size;
        };

        /**
         * @brief Represents a bin that holds free memory blocks of similar sizes.
         */
        struct Bin
        {
            FreeBlock* FreeList;
        };

        /**
         * @brief Inserts a free block into the appropriate bin based on its size.
         *
         * @param ptr Pointer to the free block.
         * @param size Size of the free block in bytes.
         */
        void InsertIntoBin(void* ptr, uint64_t size)
        {
            if (ptr == nullptr)
            {
                return;
            }

            uint32_t binIndex = FindBinIndex(size);
            FreeBlock* block = reinterpret_cast<FreeBlock*>(ptr);
            block->Size = size;
            block->Next = m_Bins[binIndex].FreeList;
            m_Bins[binIndex].FreeList = block;
        }

        /**
         * @brief Removes a free block from its corresponding bin.
         *
         * @param binIndex The index of the bin containing the block.
         * @param block The block to be removed.
         */
        void RemoveFromBin(uint32_t binIndex, FreeBlock* block)
        {
            FreeBlock** current = &m_Bins[binIndex].FreeList;
            while (*current)
            {
                if (*current == block)
                {
                    *current = block->Next;
                    return;
                }
                current = &((*current)->Next);
            }
        }

        /**
         * @brief Finds the bin index that corresponds to the given size.
         *
         * @param size Size of the memory block.
         * @return The bin index for the given size.
         */
        uint32_t FindBinIndex(uint64_t size) const
        {
            uint32_t exp = 0;
            uint32_t mantissa = 0;

            if (size < 8)
            {
                mantissa = size;
            }
            else
            {
                uint32_t leadingZeros = lzcnt_nonzero(size);
                uint32_t highestSetBit = 31 - leadingZeros;

                uint32_t mantissaStartBit = highestSetBit - 3;
                exp = mantissaStartBit + 1;
                mantissa = (size >> mantissaStartBit) & 7;

                uint32_t lowBitsMask = (1 << mantissaStartBit) - 1;

                if ((size & lowBitsMask) != 0)
                {
                    mantissa++;
                }
            }

            return (exp << 3) | mantissa;
        }

    private:
        inline static uint32_t lzcnt_nonzero(uint32_t v)
        {
#ifdef _MSC_VER
            unsigned long retVal;
            _BitScanReverse(&retVal, v);
            return 31 - retVal;
#else
            return __builtin_clz(v);
#endif
        }

        inline static uint32_t tzcnt_nonzero(uint32_t v)
        {
#ifdef _MSC_VER
            unsigned long retVal;
            _BitScanForward(&retVal, v);
            return retVal;
#else
            return __builtin_ctz(v);
#endif
        }

    private:
        static constexpr uint32_t NUM_TOP_BINS = 32;
        static constexpr uint32_t BINS_PER_LEAF = 8;
        static constexpr uint32_t TOP_BINS_INDEX_SHIFT = 3;
        static constexpr uint32_t LEAF_BINS_INDEX_MASK = 0x7;
        static constexpr uint32_t NUM_LEAF_BINS = NUM_TOP_BINS * BINS_PER_LEAF;

        void* m_Data = nullptr;
        Bin m_Bins[NUM_LEAF_BINS]; // Bin structure

        uint64_t m_Capacity = 0;
        uint64_t m_CurrentOffset = 0;
        uint64_t m_AllocatedBytes = 0;
    };
}