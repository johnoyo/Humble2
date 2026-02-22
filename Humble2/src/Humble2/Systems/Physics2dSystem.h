#pragma once

#include "Physics\Box2DPhysicsEngine.h"

#include "Scene\Scene.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"

#include <box2d\box2d.h>

namespace HBL2
{
	class HBL2_API Physics2dSystem final : public ISystem
	{
	public:
		Physics2dSystem() { Name = "Physics2dSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override {}
		virtual void OnFixedUpdate() override;
		virtual void OnDestroy() override;

	private:
		bool m_Initialized = false;
	};
}