#pragma once

#include <deque>
#include <functional>

namespace HBL2
{
	class DeletionQueue
	{
	public:
		void PushFunction(std::function<void()>&& function)
		{
			m_Deletors.push_back(function);
		}

		void Flush()
		{
			for (auto it = m_Deletors.rbegin(); it != m_Deletors.rend(); it++)
			{
				(*it)();
			}

			m_Deletors.clear();
		}

	private:
		std::deque<std::function<void()>> m_Deletors;
	};
}