#pragma once

#include "BaseAllocator.h"
#include "Utilities\Log.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
	/**
	 * @brief A high-performance memory allocator that follows the bump allocation strategy.
	 *
	 * Allocates memory in a linear fashion without individual deallocation, making it
	 * extremely fast. Memory is reset in bulk using @ref Invalidate.
	 */
	class BumpAllocator final : public BaseAllocator<BumpAllocator>
	{
	public:
		BumpAllocator() = default;

		/**
		 * @brief Initializes a bump allocator with a given memory size.
		 *
		 * @param size The total size of the memory pool in bytes.
		 */
		BumpAllocator(uint64_t size)
		{
			Initialize(size);
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
			constexpr uint64_t alignment = alignof(T);

			uint64_t alignedOffset = (m_CurrentOffset + (alignment ? alignment - 1 : 0)) & ~(alignment ? alignment - 1 : 0);

			if (alignedOffset + size > m_Capacity)
			{
				HBL2_CORE_ERROR("BumpAllocator out of memory!");
				return nullptr;
			}

			T* ptr = (T*)((char*)m_Data + alignedOffset);

			m_CurrentOffset = alignedOffset + size;

			return ptr;
		}

		/**
		 * @brief Deallocation is not supported in bump allocation.
		 *
		 * Memory is only reset in bulk via @ref Invalidate.
		 *
		 * @tparam T The type of object to deallocate.
		 * @param object The object to "deallocate" (ignored).
		 */
		template<typename T>
		void Deallocate(T* object)
		{
			// No individual deallocation, it happens at bulk.
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
		}

		/**
		 * @brief Resets the allocator, clearing all allocated memory.
		 *
		 * Does not free memory, but resets the offset.
		 */
		virtual void Invalidate() override
		{
            std::memset(m_Data, 0, m_CurrentOffset);
			m_CurrentOffset = 0;
		}

		/**
		 * @brief Frees all allocated memory and resets the allocator.
		 *
		 * After calling this, the allocator cannot be used until reinitialized.
		 */
		virtual void Free() override
		{
			::operator delete(m_Data);
			m_CurrentOffset = 0;
			m_Capacity = 0;
		}

		float GetFullPercentage()
		{
			return ((float)m_CurrentOffset / (float)m_Capacity) * 100.f;
		}

	private:
		void* m_Data = nullptr;
		uint64_t m_Capacity = 0; // In bytes
		uint64_t m_CurrentOffset = 0; // In bytes
	};
}