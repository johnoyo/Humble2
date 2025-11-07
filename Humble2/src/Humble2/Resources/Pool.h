#pragma once

#include "Core/Allocators.h"

#include "Handle.h"
#include "Utilities\Collections\Span.h"
#include "Utilities\Collections\Stack.h"

namespace HBL2
{
	template <typename T, typename H>
	class Pool
	{
	public:
		Pool()
		{
			m_Data = Allocator::Persistent.AllocateObjects<T>(m_Size);
			m_GenerationalCounter = Allocator::Persistent.Allocate<uint16_t>(sizeof(uint16_t) * m_Size);

			// std::memset(m_Data, 0, sizeof(m_Data));

			for (uint32_t i = 0; i < m_Size; i++)
			{
				m_GenerationalCounter[i] = 1;
			}

			for (int32_t i = (m_Size - 1); i >= 0; i--)
			{
				m_FreeList.Push((uint16_t)i);
			}
		}

		Handle<H> Insert(const T& data)
		{
			if (m_FreeList.Empty())
			{
				ReAllocate();
			}

			uint16_t index = m_FreeList.Top();

			m_FreeList.Pop();

			m_Data[index] = data;

			return { index, m_GenerationalCounter[index] };
		}

		void Remove(Handle<H> handle)
		{
			if (!handle.IsValid())
			{
				return;
			}

			m_GenerationalCounter[handle.m_ArrayIndex]++;
			m_FreeList.Push(handle.m_ArrayIndex);
		}

		T* Get(Handle<H> handle) const
		{
			if (!handle.IsValid())
			{
				 return nullptr;
			}

			if (handle.m_GenerationalCounter != m_GenerationalCounter[handle.m_ArrayIndex])
			{
				return nullptr;
			}

			return &m_Data[handle.m_ArrayIndex];
		}

		const Span<T> GetDataPool()
		{
			return { m_Data, m_FreeList.Top() };
		}

		Handle<H> GetHandleFromIndex(uint16_t index) { return { index, m_GenerationalCounter[index] }; }

	private:
		void ReAllocate()
		{
			// Resize data array
			T* oldData = m_Data;

			m_Data = Allocator::Persistent.AllocateObjects<T>(m_Size * 2U);

			// std::memset(m_Data, 0, sizeof(m_Data));

			for (uint32_t i = 0; i < m_Size; i++)
			{
				m_Data[i] = oldData[i];
			}

			Allocator::Persistent.Deallocate(oldData);

			// Resize generational counter array
			uint16_t* oldGenerationalCounter = m_GenerationalCounter;

			m_GenerationalCounter = Allocator::Persistent.Allocate<uint16_t>(sizeof(uint16_t) * m_Size * 2U);

			for (uint32_t i = m_Size; i < m_Size * 2; i++)
			{
				m_GenerationalCounter[i] = 1;
			}

			for (uint32_t i = 0; i < m_Size; i++)
			{
				m_GenerationalCounter[i] = oldGenerationalCounter[i];
			}

			Allocator::Persistent.Deallocate(oldGenerationalCounter);

			// Resize free list stack
			while (!m_FreeList.Empty())
			{
				m_FreeList.Pop();
			}

			for (uint32_t i = (m_Size * 2) - 1; i >= m_Size; i--)
			{
				m_FreeList.Push((uint16_t)i);
			}

			// Double size
			m_Size = m_Size * 2U;
		}

	private:
		Stack<uint16_t> m_FreeList;
		T* m_Data;
		uint16_t* m_GenerationalCounter;
		uint32_t m_Size = 32;
	};
}