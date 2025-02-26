#pragma once

#include "BaseAllocator.h"
#include "Utilities\Log.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
	class FreeListAllocator final : public BaseAllocator<FreeListAllocator>
	{
	public:
		FreeListAllocator(uint64_t size)
			: m_Capacity(size), m_CurrentOffset(0), m_FreeList(nullptr)
		{
			m_Data = ::operator new(m_Capacity);
			std::memset(m_Data, 0, m_Capacity);
		}

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

		virtual void Invalidate() override
		{
			std::memset(m_Data, 0, m_Capacity);
			m_CurrentOffset = 0;
			m_FreeList = nullptr;
		}

		virtual void Free() override
		{
			::operator delete(m_Data);

			m_Capacity = 0;
			m_CurrentOffset = 0;
			m_FreeList = nullptr;
		}

	private:
		struct FreeBlock
		{
			FreeBlock* Next;
			uint16_t Size;
		};

	private:
		void* m_Data;
		FreeBlock* m_FreeList;
		uint16_t* m_GenerationalCounter;
		uint64_t m_Capacity; // In bytes
		uint64_t m_CurrentOffset; // In bytes
	};
}