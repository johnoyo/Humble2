#pragma once

#include "Base.h"

#include <vector>
#include <initializer_list>

namespace HBL2
{
	namespace UI
	{
		class Body
		{
		public:
			Body()
				: m_Contents(nullptr)
			{
			}

			Body(const std::function<void(void)>&& contents)
				: m_Contents(contents)
			{
				HBL2_CORE_INFO("Body ctor called");
			}

			void Render()
			{
				m_Contents();
			}

		private:
			std::function<void(void)> m_Contents = nullptr;
		};
	}
}