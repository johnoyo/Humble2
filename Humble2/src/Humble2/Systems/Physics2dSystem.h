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
		Physics::ID CreateRigidbody(entt::entity entity, Component::Rigidbody2D& rb2d, Component::Transform& transform);
		Physics::ID CreateBoxCollider(entt::entity entity, Component::BoxCollider2D& bc2d, Component::Rigidbody2D& rb2d, Component::Transform& transform);

	private:
		Box2DPhysicsEngine* m_PhysicsEngine = nullptr;
		b2WorldId m_PhysicsWorld = {};
		bool m_Initialized = false;
	};
}