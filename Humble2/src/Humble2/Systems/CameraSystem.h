#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"

namespace HBL2
{
	class HBL2_API CameraSystem final : public ISystem
	{
	public:
		CameraSystem() { Name = "CameraSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
	};
}