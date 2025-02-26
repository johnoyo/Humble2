#pragma once

#include "Scene\Scene.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"

#include <box2d\box2d.h>

namespace HBL2
{
	class HBL2_API Physics2dSystem final : public ISystem
	{
	public:
		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnDestroy() override;

	private:
		b2WorldId m_PhysicsWorld = {};
		int m_SubStepCount = 4;
		float m_GravityForce = -9.81f;
	};
}