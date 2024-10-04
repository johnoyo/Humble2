#pragma once

#include "Scene\ISystem.h"

namespace HBL2
{
	class LightSystem final : public ISystem
	{
	public:
		LightSystem() { Name = "LightSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnDestroy() override;
	};
}