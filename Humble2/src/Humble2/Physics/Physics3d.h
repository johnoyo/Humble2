#pragma once

#include "Physics.h"
#include "Humble2API.h"
#include "Scene/Components.h"

#include <entt.hpp>
#include <glm/glm.hpp>

#include <functional>

namespace HBL2
{
	namespace Physics3D
	{
		enum class HBL2_API CollisionEventType
		{
			Enter = 0,
			Stay,
			Exit,
		};

		struct CollisionEnterEvent
		{
			entt::entity entityA;
			entt::entity entityB;
		};

		struct CollisionStayEvent
		{
			entt::entity entityA;
			entt::entity entityB;
		};

		struct CollisionExitEvent
		{
			entt::entity entityA;
			entt::entity entityB;
		};

		struct TriggerEnterEvent
		{
			entt::entity entityA;
			entt::entity entityB;
		};

		struct TriggerStayEvent
		{
			entt::entity entityA;
			entt::entity entityB;
		};

		struct TriggerExitEvent
		{
			entt::entity entityA;
			entt::entity entityB;
		};

		HBL2_API void AddLinearVelocity(Component::Rigidbody& rb, const glm::vec3& linearVelocity);
		HBL2_API void SetLinearVelocity(Component::Rigidbody& rb, const glm::vec3& linearVelocity);
		HBL2_API glm::vec3 GetLinearVelocity(Component::Rigidbody& rb);

		HBL2_API void AddAngularVelocity(Component::Rigidbody& rb, const glm::vec3& angularVelocity);
		HBL2_API void SetAngularVelocity(Component::Rigidbody& rb, const glm::vec3& angularVelocity);
		HBL2_API glm::vec3 GetAngularVelocity(Component::Rigidbody& rb);

		HBL2_API void ApplyForce(Component::Rigidbody& rb, const glm::vec3& force);
		HBL2_API void ApplyTorque(Component::Rigidbody& rb, const glm::vec3& torque);
		HBL2_API void ApplyImpulse(Component::Rigidbody& rb, const glm::vec3& impluse);
		HBL2_API void ApplyAngularImpulse(Component::Rigidbody& rb, const glm::vec3& angularImpulse);

		HBL2_API void OnCollisionEnterEvent(std::function<void(CollisionEnterEvent*)>&& enterEventFunc);
		HBL2_API void OnCollisionStayEvent(std::function<void(CollisionStayEvent*)>&& stayEventFunc);
		HBL2_API void OnCollisionExitEvent(std::function<void(CollisionExitEvent*)>&& exitEventFunc);

		HBL2_API void OnTriggerEnterEvent(std::function<void(TriggerEnterEvent*)>&& enterEventFunc);
		HBL2_API void OnTriggerStayEvent(std::function<void(TriggerStayEvent*)>&& stayEventFunc);
		HBL2_API void OnTriggerExitEvent(std::function<void(TriggerExitEvent*)>&& exitEventFunc);

		HBL2_API void DispatchCollisionEvents(CollisionEventType collisionEventType, void* collisionEventData);
		HBL2_API void DispatchTriggerEvents(CollisionEventType collisionEventType, void* triggerEventData);
		HBL2_API void ClearCollisionEvents();
		HBL2_API void ClearTriggerEvents();
	}
}