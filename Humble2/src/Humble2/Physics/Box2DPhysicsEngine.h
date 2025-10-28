#pragma once

#include "PhysicsEngine2D.h"

#include <box2d\box2d.h>

namespace HBL2
{
	class Box2DPhysicsEngine final : public PhysicsEngine2D
	{
	public:
		void Initialize();
		void Step(float timeStep);
		void Shutdown();

		void DispatchCollisionEvent(Physics::CollisionEventType collisionEventType, void* collisionEventData);
		void DispatchTriggerEvent(Physics::CollisionEventType collisionEventType, void* triggerEventData);

		b2WorldId Get() { return m_PhysicsWorld; }

		virtual void OnCollisionEnterEvent(std::function<void(Physics::CollisionEnterEvent*)>&& enterEventFunc) override;
		virtual void OnCollisionStayEvent(std::function<void(Physics::CollisionStayEvent*)>&& stayEventFunc) override;
		virtual void OnCollisionExitEvent(std::function<void(Physics::CollisionExitEvent*)>&& exitEventFunc) override;
		virtual void OnCollisionHitEvent(std::function<void(Physics::CollisionHitEvent*)>&& hitEventFunc) override;
		virtual void OnTriggerEnterEvent(std::function<void(Physics::TriggerEnterEvent*)>&& enterEventFunc) override;
		virtual void OnTriggerStayEvent(std::function<void(Physics::TriggerStayEvent*)>&& stayEventFunc) override;
		virtual void OnTriggerExitEvent(std::function<void(Physics::TriggerExitEvent*)>&& exitEventFunc) override;

		virtual void Teleport(Component::Rigidbody2D& rb2d, const glm::vec2& position, glm::vec2& rotation) override;

		virtual void ApplyForce(Component::Rigidbody2D& rb2d, const glm::vec2& force, bool wake) override;
		virtual void ApplyForce(Component::Rigidbody2D& rb2d, const glm::vec2& force, const glm::vec2& worldPosition, bool wake) override;
		virtual void ApplyLinearImpulse(Component::Rigidbody2D& rb2d, const glm::vec2& velocity, bool wake) override;
		virtual void ApplyLinearImpulse(Component::Rigidbody2D& rb2d, const glm::vec2& velocity, const glm::vec2& worldPosition, bool wake) override;
		virtual void ApplyAngularImpulse(Component::Rigidbody2D& rb2d, float impulse, bool wake) override;
		
		virtual void SetLinearVelocity(Component::Rigidbody2D& rb2d, const glm::vec2& velocity) override;
		virtual glm::vec2 GetLinearVelocity(Component::Rigidbody2D& rb2d) override;
		virtual void SetAngularVelocity(Component::Rigidbody2D& rb2d, float velocity) override;
		virtual float GetAngularVelocity(Component::Rigidbody2D& rb2d) override;

		virtual void SetDebugDrawEnabled(bool enabled) override;
		virtual void OnDebugDraw() override;

	private:
		b2WorldId m_PhysicsWorld = {};
		int m_SubStepCount = 4;
		float m_GravityForce = -9.81f;

		std::vector<std::function<void(Physics::CollisionEnterEvent*)>> m_CollisionEnterEvents;
		std::vector<std::function<void(Physics::CollisionExitEvent*)>> m_CollisionExitEvents;
		std::vector<std::function<void(Physics::CollisionHitEvent*)>> m_CollisionHitEvents;

		std::vector<std::function<void(Physics::TriggerEnterEvent*)>> m_TriggerEnterEvents;
		std::vector<std::function<void(Physics::TriggerExitEvent*)>> m_TriggerExitEvents;

		bool m_DebugDrawEnabled = false;
		b2DebugDraw m_DebugDraw;
	};
}