#pragma once

#include "Physics/JoltPhysicsEngine.h"
#include "Scene\Scene.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"

namespace HBL2
{
	class HBL2_API Physics3dSystem final : public ISystem
	{
	public:
		Physics3dSystem() { Name = "Physics3dSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnFixedUpdate() override;
		virtual void OnDestroy() override;

	private:
		bool m_Initialized = false;
	};
}