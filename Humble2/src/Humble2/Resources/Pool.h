#pragma once

#include "Handle.h"
#include "Utilities\Collections\Span.h"

#include <stack>

namespace HBL2
{
	template <typename T, typename H>
	class Pool
	{
	public:
		Pool()
		{
			m_Data = new T[m_Size];
			m_GenerationalCounter = new uint16_t[m_Size];

			memset(m_Data, 0, sizeof(m_Data));

			for (uint32_t i = 0; i < m_Size; i++)
			{
				m_GenerationalCounter[i] = 1;
			}

			for (int32_t i = (m_Size - 1); i >= 0; i--)
			{
				m_FreeList.push((uint16_t)i);
			}
		}

		Handle<H> Insert(const T& data)
		{
			if (m_FreeList.empty())
			{
				ReAllocate();
			}

			uint16_t index = m_FreeList.top();

			m_FreeList.pop();

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
			m_FreeList.push(handle.m_ArrayIndex);
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
			return { m_Data, m_FreeList.top() };
		}

		Handle<H> GetHandleFromIndex(uint16_t index) { return { index, m_GenerationalCounter[index] }; }

	private:
		void ReAllocate()
		{
			// Resize data array
			T* oldData = m_Data;

			m_Data = new T[m_Size * (uint32_t)2];
			memset(m_Data, 0, sizeof(m_Data));

			for (uint32_t i = 0; i < m_Size; i++)
			{
				m_Data[i] = oldData[i];
			}

			delete[] oldData;

			// Resize generational counter array
			uint16_t* oldGenerationalCounter = m_GenerationalCounter;

			m_GenerationalCounter = new uint16_t[m_Size * (uint32_t)2];

			for (uint32_t i = m_Size; i < m_Size * 2; i++)
			{
				m_GenerationalCounter[i] = 1;
			}

			for (uint32_t i = 0; i < m_Size; i++)
			{
				m_GenerationalCounter[i] = oldGenerationalCounter[i];
			}

			delete[] oldGenerationalCounter;

			// Resize free list stack
			while (!m_FreeList.empty())
			{
				m_FreeList.pop();
			}

			for (uint32_t i = (m_Size * 2) - 1; i >= m_Size; i--)
			{
				m_FreeList.push((uint16_t)i);
			}

			// Double size
			m_Size = m_Size * 2;
		}

	private:
		std::stack<uint16_t> m_FreeList;
		T* m_Data;
		uint16_t* m_GenerationalCounter;
		uint32_t m_Size = 32;
	};
}