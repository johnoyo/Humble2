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
			m_Data = new T[20];
			m_GenerationalCounter = new uint16_t[20];

			memset(m_Data, 0, sizeof(m_Data));
			memset(m_GenerationalCounter, 0, sizeof(m_GenerationalCounter));

			for (int16_t i = 19; i >= 0; i--)
			{
				m_FreeList.push((uint16_t)i);
			}
		}

		Handle<H> Insert(const T& data)
		{
			// TODO: Handle resizing.

			uint16_t index = m_FreeList.top();

			m_FreeList.pop();

			m_Data[index] = data;
			m_GenerationalCounter[index] = 1;

			return { index, m_GenerationalCounter[index] };
		}

		void Remove(Handle<H> handle)
		{
			handle.m_GenerationalCounter++;
			m_FreeList.push(handle.m_ArrayIndex);
		}

		T* Get(Handle<H> handle) const
		{
			if (handle.m_GenerationalCounter != m_GenerationalCounter[handle.m_ArrayIndex])
				return nullptr;

			return &m_Data[handle.m_ArrayIndex];
		}

	private:
		std::stack<uint16_t> m_FreeList;
		T* m_Data;
		uint16_t* m_GenerationalCounter;
		uint32_t m_CurrentSize = 20;
	};
}