#pragma once

#include "Humble2.h"

namespace HBL2Runtime
{
	class RuntimeContext final : public HBL2::Context
	{
	public:
		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnGuiRender(float ts) override;
		virtual void OnDestroy() override;
	private:
		bool OpenProject();
		HBL2::Scene* m_ActiveScene = nullptr;
	};
}
