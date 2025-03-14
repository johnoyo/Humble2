#pragma once

#include "BaseAllocator.h"
#include "Utilities\Log.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
	/// <summary>
	/// A high-performance memory allocator that follows the bump allocation strategy.
	/// Allocates memory in a linear fashion without individual deallocation, making it 
	/// extremely fast. Memory is reset in bulk using <see cref="Invalidate"/>.
	/// </summary>
	class BumpAllocator final : public BaseAllocator<BumpAllocator>
	{
	public:
		/// <summary>
		/// Initializes a bump allocator with a given memory size.
		/// </summary>
		/// <param name="size">The total size of the memory pool in bytes.</param>
		BumpAllocator(uint64_t size)
			: m_Capacity(size), m_CurrentOffset(0)
		{
			m_Data = ::operator new(m_Capacity);
			std::memset(m_Data, 0, m_Capacity);
		}

		/// <summary>
		/// Allocates a block of memory of the given size.
		/// </summary>
		/// <typeparam name="T">The type of object to allocate.</typeparam>
		/// <param name="size">The size of the allocation in bytes (default: sizeof(T)).</param>
		/// <returns>A pointer to the allocated memory, or nullptr if out of memory.</returns>
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

		/// <summary>
		/// Deallocation is not supported in bump allocation.
		/// Memory is only reset in bulk via <see cref="Invalidate"/>.
		/// </summary>
		/// <typeparam name="T">The type of object to deallocate.</typeparam>
		/// <param name="object">The object to "deallocate" (ignored).</param>
		template<typename T>
		void Deallocate(T* object)
		{
			// No individual deallocation, it happens at bulk.
		}

		/// <summary>
		/// Resets the allocator, clearing all allocated memory.
		/// Does not free memory, but sets all bytes to zero and resets the offset.
		/// </summary>
		virtual void Invalidate() override
		{
			std::memset(m_Data, 0, m_Capacity);
			m_CurrentOffset = 0;
		}

		/// <summary>
		/// Frees all allocated memory and resets the allocator.
		/// After calling this, the allocator cannot be used until reinitialized.
		/// </summary>
		virtual void Free() override
		{
			::operator delete(m_Data);
			m_CurrentOffset = 0;
			m_Capacity = 0;
		}

	private:
		void* m_Data;
		uint64_t m_Capacity; // In bytes
		uint64_t m_CurrentOffset; // In bytes
	};
}