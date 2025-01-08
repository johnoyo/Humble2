#pragma once

#include "Humble2.h"

namespace HBL2
{
	namespace Runtime
	{
		class RuntimeContext final : public Context
		{
		public:
			virtual void OnCreate() override;
			virtual void OnUpdate(float ts) override;
			virtual void OnGuiRender(float ts) override;
			virtual void OnDestroy() override;
		private:
			bool OpenProject();
			Scene* m_ActiveScene = nullptr;
		};
	}
}
