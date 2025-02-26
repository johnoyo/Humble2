#pragma once

#include "BaseAllocator.h"
#include "Utilities\Log.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
	class ArenaAllocator final : public BaseAllocator<ArenaAllocator>
	{
	public:
		ArenaAllocator(uint64_t size)
			: m_Capacity(size), m_CurrentOffset(0)
		{
			m_Data = ::operator new(m_Capacity);
			std::memset(m_Data, 0, m_Capacity);
		}

		template<typename T>
		T* Allocate(uint64_t size = sizeof(T))
		{
			constexpr uint64_t alignment = alignof(T);

			uint64_t alignedOffset = (m_CurrentOffset + (alignment ? alignment - 1 : 0)) & ~(alignment ? alignment - 1 : 0);

			if (alignedOffset + size > m_Capacity)
			{
				HBL2_CORE_ERROR("ArenaAllocator out of memory!");
				return nullptr;
			}

			T* ptr = (T*)((char*)m_Data + alignedOffset);

			m_CurrentOffset = alignedOffset + size;

			return ptr;
		}

		template<typename T>
		void Deallocate(T* object)
		{
			// No individual deallocation, it happens at bulk.
		}

		virtual void Invalidate() override
		{
			std::memset(m_Data, 0, m_Capacity);
			m_CurrentOffset = 0;
		}

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