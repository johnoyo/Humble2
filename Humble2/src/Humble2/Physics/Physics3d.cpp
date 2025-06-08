#include "Physics3d.h"

namespace HBL2
{
	namespace Physics3D
	{
		static std::vector<std::function<void(CollisionEnterEvent*)>> g_CollisionEnterEvents;
		static std::vector<std::function<void(CollisionStayEvent*)>> g_CollisionStayEvents;
		static std::vector<std::function<void(CollisionExitEvent*)>> g_CollisionExitEvents;

		static std::vector<std::function<void(TriggerEnterEvent*)>> g_TriggerEnterEvents;
		static std::vector<std::function<void(TriggerStayEvent*)>> g_TriggerStayEvents;
		static std::vector<std::function<void(TriggerExitEvent*)>> g_TriggerExitEvents;

		HBL2_API void AddLinearVelocity(Component::Rigidbody& rb, const glm::vec3& linearVelocity)
		{
		}

		HBL2_API void SetLinearVelocity(Component::Rigidbody& rb, const glm::vec3& linearVelocity)
		{
		}

		HBL2_API glm::vec3 GetLinearVelocity(Component::Rigidbody& rb)
		{
			return glm::vec3();
		}

		HBL2_API void AddAngularVelocity(Component::Rigidbody& rb, const glm::vec3& angularVelocity)
		{
		}

		HBL2_API void SetAngularVelocity(Component::Rigidbody& rb, const glm::vec3& angularVelocity)
		{
		}

		HBL2_API glm::vec3 GetAngularVelocity(Component::Rigidbody& rb)
		{
			return glm::vec3();
		}

		HBL2_API void ApplyForce(Component::Rigidbody& rb, const glm::vec3& force)
		{
		}

		HBL2_API void ApplyTorque(Component::Rigidbody& rb, const glm::vec3& torque)
		{
		}

		HBL2_API void ApplyImpulse(Component::Rigidbody& rb, const glm::vec3& impluse)
		{
		}

		HBL2_API void ApplyAngularImpulse(Component::Rigidbody& rb, const glm::vec3& angularImpulse)
		{
		}

		HBL2_API void OnCollisionEnterEvent(std::function<void(CollisionEnterEvent*)>&& enterEventFunc)
		{
			// TODO: This is not thread safe, add lock when system scheduling is added.
			g_CollisionEnterEvents.emplace_back(enterEventFunc);
		}

		HBL2_API void OnCollisionStayEvent(std::function<void(CollisionStayEvent*)>&& stayEventFunc)
		{
			// TODO: This is not thread safe, add lock when system scheduling is added.
			g_CollisionStayEvents.emplace_back(stayEventFunc);
		}

		HBL2_API void OnCollisionExitEvent(std::function<void(CollisionExitEvent*)>&& exitEventFunc)
		{
			// TODO: This is not thread safe, add lock when system scheduling is added.
			g_CollisionExitEvents.emplace_back(exitEventFunc);
		}

		HBL2_API void OnTriggerEnterEvent(std::function<void(TriggerEnterEvent*)>&& enterEventFunc)
		{
			// TODO: This is not thread safe, add lock when system scheduling is added.
			g_TriggerEnterEvents.emplace_back(enterEventFunc);
		}

		HBL2_API void OnTriggerStayEvent(std::function<void(TriggerStayEvent*)>&& stayEventFunc)
		{
			// TODO: This is not thread safe, add lock when system scheduling is added.
			g_TriggerStayEvents.emplace_back(stayEventFunc);
		}

		HBL2_API void OnTriggerExitEvent(std::function<void(TriggerExitEvent*)>&& exitEventFunc)
		{
			// TODO: This is not thread safe, add lock when system scheduling is added.
			g_TriggerExitEvents.emplace_back(exitEventFunc);
		}

		HBL2_API void DispatchCollisionEvents(CollisionEventType collisionEventType, void* collisionEventData)
		{
			switch (collisionEventType)
			{
			case CollisionEventType::Enter:
				if (!g_CollisionEnterEvents.empty())
				{
					CollisionEnterEvent* collisionEnterEvent = (CollisionEnterEvent*)collisionEventData;

					for (const auto& callback : g_CollisionEnterEvents)
					{
						callback(collisionEnterEvent);
					}
				}
				break;
			case CollisionEventType::Stay:
				if (!g_CollisionStayEvents.empty())
				{
					CollisionStayEvent* collisionStayEvent = (CollisionStayEvent*)collisionEventData;

					for (const auto& callback : g_CollisionStayEvents)
					{
						callback(collisionStayEvent);
					}
				}
				break;
			case CollisionEventType::Exit:
				if (!g_CollisionExitEvents.empty())
				{
					CollisionExitEvent* collisionExitEvent = (CollisionExitEvent*)collisionEventData;

					for (const auto& callback : g_CollisionExitEvents)
					{
						callback(collisionExitEvent);
					}
				}
				break;
			}
		}

		HBL2_API void DispatchTriggerEvents(CollisionEventType collisionEventType, void* triggerEventData)
		{
			switch (collisionEventType)
			{
			case CollisionEventType::Enter:
				if (!g_TriggerEnterEvents.empty())
				{
					TriggerEnterEvent* triggerEnterEvent = (TriggerEnterEvent*)triggerEventData;

					for (const auto& callback : g_TriggerEnterEvents)
					{
						callback(triggerEnterEvent);
					}
				}
				break;
			case CollisionEventType::Stay:
				if (!g_TriggerStayEvents.empty())
				{
					TriggerStayEvent* triggerStayEvent = (TriggerStayEvent*)triggerEventData;

					for (const auto& callback : g_TriggerStayEvents)
					{
						callback(triggerStayEvent);
					}
				}
				break;
			case CollisionEventType::Exit:
				if (!g_TriggerExitEvents.empty())
				{
					TriggerExitEvent* triggerExitEvent = (TriggerExitEvent*)triggerEventData;

					for (const auto& callback : g_TriggerExitEvents)
					{
						callback(triggerExitEvent);
					}
				}
				break;
			}
		}

		HBL2_API void ClearCollisionEvents()
		{
			g_CollisionEnterEvents.clear();
			g_CollisionStayEvents.clear();
			g_CollisionExitEvents.clear();
		}

		HBL2_API void ClearTriggerEvents()
		{
			g_TriggerEnterEvents.clear();
			g_TriggerStayEvents.clear();
			g_TriggerExitEvents.clear();
		}
	}
}