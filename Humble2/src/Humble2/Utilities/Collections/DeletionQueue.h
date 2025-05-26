#pragma once

#include "Deque.h"
#include <functional>

namespace HBL2
{
	class DeletionQueue
	{
	public:
		void PushFunction(std::function<void()>&& function)
		{
			std::scoped_lock lock(m_Lock);
			m_Deletors.PushBack(function);
		}

		void Flush()
		{
			for (auto it = m_Deletors.rbegin(); it != m_Deletors.rend(); it++)
			{
				(*it)();
			}

			m_Deletors.Clear();
		}

	private:
		Deque<std::function<void()>> m_Deletors;
		std::mutex m_Lock;
	};
}