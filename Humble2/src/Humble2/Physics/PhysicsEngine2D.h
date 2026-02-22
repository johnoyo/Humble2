#pragma once

#include "Scene/Scene.h"
#include "Scene/Components.h"

#include <glm/glm.hpp>

namespace HBL2
{
	class HBL2_API PhysicsEngine2D
	{
	public:
		static PhysicsEngine2D* Instance;

		PhysicsEngine2D() = default;
		virtual ~PhysicsEngine2D() = default;

		virtual void Initialize(Scene* ctx) = 0;
		virtual void Update() = 0;
		virtual void Shutdown() = 0;

		virtual void OnCollisionEnterEvent(std::function<void(Physics::CollisionEnterEvent*)>&& enterEventFunc) = 0;
		virtual void OnCollisionStayEvent(std::function<void(Physics::CollisionStayEvent*)>&& stayEventFunc) = 0;
		virtual void OnCollisionExitEvent(std::function<void(Physics::CollisionExitEvent*)>&& exitEventFunc) = 0;
		virtual void OnCollisionHitEvent(std::function<void(Physics::CollisionHitEvent*)>&& hitEventFunc) = 0;
		virtual void OnTriggerEnterEvent(std::function<void(Physics::TriggerEnterEvent*)>&& enterEventFunc) = 0;
		virtual void OnTriggerStayEvent(std::function<void(Physics::TriggerStayEvent*)>&& stayEventFunc) = 0;
		virtual void OnTriggerExitEvent(std::function<void(Physics::TriggerExitEvent*)>&& exitEventFunc) = 0;

		virtual void Teleport(Component::Rigidbody2D& rb2d, const glm::vec2& position, glm::vec2& rotation) {}

		virtual void ApplyForce(Component::Rigidbody2D& rb2d, const glm::vec2& force, bool wake) {}
		virtual void ApplyForce(Component::Rigidbody2D& rb2d, const glm::vec2& force, const glm::vec2& worldPosition, bool wake) {}
		virtual void ApplyLinearImpulse(Component::Rigidbody2D& rb2d, const glm::vec2& velocity, bool wake) {}
		virtual void ApplyLinearImpulse(Component::Rigidbody2D& rb2d, const glm::vec2& velocity, const glm::vec2& worldPosition, bool wake) {}
		virtual void ApplyAngularImpulse(Component::Rigidbody2D& rb2d, float impulse, bool wake) {}

		virtual void SetLinearVelocity(Component::Rigidbody2D& rb2d, const glm::vec2& velocity) {}
		virtual glm::vec2 GetLinearVelocity(Component::Rigidbody2D& rb2d) { return {}; }
		virtual void SetAngularVelocity(Component::Rigidbody2D& rb2d, float velocity) {}
		virtual float GetAngularVelocity(Component::Rigidbody2D& rb2d) { return 0.0f; }

		virtual void SetDebugDrawEnabled(bool enabled) {}
		virtual void OnDebugDraw() {}
	};
}