#pragma once

#include "BaseAllocator.h"
#include "Utilities\Log.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
	/**
	 * @brief A memory allocator that utilizes a free list for efficient memory reuse.
	 *
	 * When memory is allocated, it searches for a suitable free block in the free list.
	 * If no suitable block is found, memory is allocated sequentially from a pre-allocated buffer.
	 *
	 * When memory is deallocated, it is placed back into the free list, allowing future allocations
	 * to reuse previously freed memory, reducing fragmentation.
	 *
	 * This allocator is particularly useful for scenarios with frequent small allocations
	 * and deallocations, where avoiding system-level memory allocation overhead is beneficial.
	 */
	class FreeListAllocator final : public BaseAllocator<FreeListAllocator>
	{
	public:
		/**
		 * @brief Initializes the allocator with a specified memory size.
		 *
		 * @param size The total size of the memory pool in bytes.
		 */
		FreeListAllocator(uint64_t size)
			: m_Capacity(size), m_CurrentOffset(0), m_FreeList(nullptr)
		{
			m_Data = ::operator new(m_Capacity);
			std::memset(m_Data, 0, m_Capacity);
		}

		/**
		 * @brief Allocates a block of memory of the given size.
		 *
		 * If a suitable free block is available, it is reused.
		 * Otherwise, a new block is allocated from the memory pool.
		 *
		 * @tparam T The type of object to allocate.
		 * @param size The size of the allocation in bytes (default: sizeof(T)).
		 * @return A pointer to the allocated memory, or nullptr if out of memory.
		 */
		template<typename T>
		T* Allocate(uint64_t size = sizeof(T))
		{
			if (m_FreeList)
			{
				FreeBlock* freeBlock = m_FreeList;
				FreeBlock* prevOfFreeBlock = nullptr;

				while (freeBlock != nullptr)
				{
					if (freeBlock->Size >= size)
					{
						if (prevOfFreeBlock != nullptr)
						{
							prevOfFreeBlock->Next = freeBlock->Next;
						}

						return (T*)freeBlock;
					}

					prevOfFreeBlock = freeBlock;
					freeBlock = freeBlock->Next;
				}
			}

			constexpr uint64_t alignment = alignof(T);

			uint64_t alignedOffset = (m_CurrentOffset + (alignment ? alignment - 1 : 0)) & ~(alignment ? alignment - 1 : 0);

			if (alignedOffset + size > m_Capacity)
			{
				HBL2_CORE_ERROR("FreeListAllocator out of memory!");
				return nullptr;
			}

			T* ptr = (T*)((char*)m_Data + alignedOffset);

			m_CurrentOffset = alignedOffset + size;

			return ptr;
		}

		/**
		 * @brief Deallocates a previously allocated block by adding it to the free list.
		 *
		 * The block can be reused in future allocations.
		 *
		 * @tparam T The type of object being deallocated.
		 * @param object The pointer to the memory block being freed.
		 */
		template<typename T>
		void Deallocate(T* object)
		{
			if (object == nullptr)
			{
				return;
			}

			FreeBlock* block = (FreeBlock*)object;
			block->Next = m_FreeList;
			block->Size = sizeof(T);
			m_FreeList = block;
		}

		/**
		 * @brief Resets the allocator, clearing all allocated memory.
		 *
		 * This does not free the memory pool but marks all memory as available.
		 */
		virtual void Invalidate() override
		{
			std::memset(m_Data, 0, m_Capacity);
			m_CurrentOffset = 0;
			m_FreeList = nullptr;
		}

		/**
		 * @brief Frees all allocated memory and resets the allocator.
		 *
		 * After calling this, the allocator cannot be used until reinitialized.
		 */
		virtual void Free() override
		{
			::operator delete(m_Data);

			m_Capacity = 0;
			m_CurrentOffset = 0;
			m_FreeList = nullptr;
		}

	private:
		/**
		 * @brief Represents a free memory block in the free list.
		 */
		struct FreeBlock
		{
			FreeBlock* Next;
			uint16_t Size;
		};

	private:
		void* m_Data;
		FreeBlock* m_FreeList;
		uint64_t m_Capacity; // In bytes
		uint64_t m_CurrentOffset; // In bytes
	};
}