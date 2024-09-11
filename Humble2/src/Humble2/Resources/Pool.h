#pragma once

#include "Handle.h"

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
			memset(m_GenerationalCounter, 0, sizeof(m_GenerationalCounter));

			for (int32_t i = (m_Size - 1); i >= 0; i--)
			{
				m_FreeList.push((uint16_t)i);
			}
		}

		Handle<H> Insert(const T& data)
		{
			if (m_CurrentSize >= m_Size)
			{
				ReAllocate();
			}

			uint16_t index = m_FreeList.top();

			m_FreeList.pop();

			m_Data[index] = data;
			m_GenerationalCounter[index] = 1;

			m_CurrentSize++;

			return { index, m_GenerationalCounter[index] };
		}

		void Remove(Handle<H> handle)
		{
			m_CurrentSize--;
			handle.m_GenerationalCounter++;
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

	private:
		void ReAllocate()
		{
			// Resize data array
			T* oldData = m_Data;

			m_Data = new T[m_Size * 2];
			memset(m_Data, 0, sizeof(m_Data));
			memcpy(m_Data, oldData, sizeof(oldData));

			delete oldData;

			// Resize generational counter array
			uint16_t* oldGenerationalCounter = m_GenerationalCounter;

			m_GenerationalCounter = new uint16_t[m_Size * 2];

			memset(m_GenerationalCounter, 0, sizeof(m_GenerationalCounter));
			memcpy(m_GenerationalCounter, oldGenerationalCounter, sizeof(oldGenerationalCounter));

			delete oldGenerationalCounter;

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
		uint32_t m_CurrentSize = 0;
		uint32_t m_Size = 32;
	};
}