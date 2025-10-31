#pragma once

#include "Scene/Components.h"

#include <glm/glm.hpp>

namespace HBL2
{
	class HBL2_API PhysicsEngine3D
	{
	public:
		static PhysicsEngine3D* Instance;

		PhysicsEngine3D() = default;
		virtual ~PhysicsEngine3D() = default;

		virtual void OnCollisionEnterEvent(std::function<void(Physics::CollisionEnterEvent*)>&& enterEventFunc) = 0;
		virtual void OnCollisionStayEvent(std::function<void(Physics::CollisionStayEvent*)>&& stayEventFunc) = 0;
		virtual void OnCollisionExitEvent(std::function<void(Physics::CollisionExitEvent*)>&& exitEventFunc) = 0;
		virtual void OnTriggerEnterEvent(std::function<void(Physics::TriggerEnterEvent*)>&& enterEventFunc) = 0;
		virtual void OnTriggerStayEvent(std::function<void(Physics::TriggerStayEvent*)>&& stayEventFunc) = 0;
		virtual void OnTriggerExitEvent(std::function<void(Physics::TriggerExitEvent*)>&& exitEventFunc) = 0;

		virtual void SetPosition(Component::Rigidbody& rb, const glm::vec3& position) {}
		virtual void SetRotation(Component::Rigidbody& rb, const glm::vec3& rotation) {}

		virtual void AddLinearVelocity(Component::Rigidbody& rb, const glm::vec3& linearVelocity) {}
		virtual void SetLinearVelocity(Component::Rigidbody& rb, const glm::vec3& linearVelocity) {}
		virtual glm::vec3 GetLinearVelocity(Component::Rigidbody& rb) { return {}; }

		virtual void SetAngularVelocity(Component::Rigidbody& rb, const glm::vec3& angularVelocity) {}
		virtual glm::vec3 GetAngularVelocity(Component::Rigidbody& rb) { return {}; }

		virtual void ApplyForce(Component::Rigidbody& rb, const glm::vec3& force) {}
		virtual void ApplyTorque(Component::Rigidbody& rb, const glm::vec3& torque) {}
		virtual void ApplyImpulse(Component::Rigidbody& rb, const glm::vec3& impluse) {}
		virtual void ApplyAngularImpulse(Component::Rigidbody& rb, const glm::vec3& angularImpulse) {}

		virtual void SetDebugDrawEnabled(bool enabled) {}
		virtual void ShowColliders(bool show) {}
		virtual void ShowBoundingBoxes(bool show) {}
		virtual void OnDebugDraw() {}
	};
}