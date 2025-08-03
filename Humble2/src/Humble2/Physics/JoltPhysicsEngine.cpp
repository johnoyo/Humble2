#include "JoltPhysicsEngine.h"

namespace HBL2
{
	static inline const JPH::BodyID& GetBodyIDFromPhysicsID(Physics::ID id)
	{
		return *((JPH::BodyID*)id);
	}

	static inline Physics::ID GetPhysicsIDFromBodyID(const JPH::BodyID& bodyId)
	{
		return (Physics::ID)&bodyId;
	}

	static void TraceImpl(const char* inFMT, ...)
	{
		// Format the message
		va_list list;
		va_start(list, inFMT);
		char buffer[1024];
		vsnprintf(buffer, sizeof(buffer), inFMT, list);
		va_end(list);

		// Print to the TTY
		HBL2_CORE_TRACE(buffer);
	}

	void JoltPhysicsEngine::Initialize()
	{
		JPH::RegisterDefaultAllocator();
		JPH::Trace = TraceImpl;
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();

		m_TempAllocator = new JPH::TempAllocatorImpl(50_MB);
		m_JobSystem.Init(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, -1);

		const uint32_t cMaxBodies = 65536;
		const uint32_t cNumBodyMutexes = 0;
		const uint32_t cMaxBodyPairs = 65536;
		const uint32_t cMaxContactConstraints = 10240;

		m_PhysicsSystem = new JPH::PhysicsSystem;

		m_PhysicsSystem->Init(cMaxBodies,
			cNumBodyMutexes,
			cMaxBodyPairs,
			cMaxContactConstraints,
			m_BroadPhaseLayerInterface,
			m_ObjectVsBroadPhaseLayerFilter,
			m_ObjectVsObjectLayerFilter);
	}

	void JoltPhysicsEngine::Step(float inDeltaTime, int inCollisionSteps)
	{
		m_PhysicsSystem->Update(inDeltaTime, inCollisionSteps, m_TempAllocator, &m_JobSystem);
	}

	void JoltPhysicsEngine::ShutDown()
	{
		// Clear collision events.
		m_CollisionEnterEvents.clear();
		m_CollisionStayEvents.clear();
		m_CollisionExitEvents.clear();

		// Clear trigger events.
		m_TriggerEnterEvents.clear();
		m_TriggerStayEvents.clear();
		m_TriggerExitEvents.clear();

		// Unregisters all types with the factory and cleans up the default material.
		JPH::UnregisterTypes();

		// Destroy temp allocator.
		delete m_TempAllocator;
		m_TempAllocator = nullptr;

		// Destroy the factory.
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;

		// Delete physics system.
		delete m_PhysicsSystem;
	}

	void JoltPhysicsEngine::DispatchCollisionEvent(Physics::CollisionEventType collisionEventType, void* collisionEventData)
	{
		switch (collisionEventType)
		{
		case Physics::CollisionEventType::Enter:
			if (!m_CollisionEnterEvents.empty())
			{
				Physics::CollisionEnterEvent* collisionEnterEvent = (Physics::CollisionEnterEvent*)collisionEventData;

				for (const auto& callback : m_CollisionEnterEvents)
				{
					callback(collisionEnterEvent);
				}
			}
			break;
		case Physics::CollisionEventType::Stay:
			if (!m_CollisionStayEvents.empty())
			{
				Physics::CollisionStayEvent* collisionStayEvent = (Physics::CollisionStayEvent*)collisionEventData;

				for (const auto& callback : m_CollisionStayEvents)
				{
					callback(collisionStayEvent);
				}
			}
			break;
		case Physics::CollisionEventType::Exit:
			if (!m_CollisionExitEvents.empty())
			{
				Physics::CollisionExitEvent* collisionExitEvent = (Physics::CollisionExitEvent*)collisionEventData;

				for (const auto& callback : m_CollisionExitEvents)
				{
					callback(collisionExitEvent);
				}
			}
			break;
		}
	}

	void JoltPhysicsEngine::DispatchTriggerEvent(Physics::CollisionEventType collisionEventType, void* triggerEventData)
	{
		switch (collisionEventType)
		{
		case Physics::CollisionEventType::Enter:
			if (!m_TriggerEnterEvents.empty())
			{
				Physics::TriggerEnterEvent* triggerEnterEvent = (Physics::TriggerEnterEvent*)triggerEventData;

				for (const auto& callback : m_TriggerEnterEvents)
				{
					callback(triggerEnterEvent);
				}
			}
			break;
		case Physics::CollisionEventType::Stay:
			if (!m_TriggerStayEvents.empty())
			{
				Physics::TriggerStayEvent* triggerStayEvent = (Physics::TriggerStayEvent*)triggerEventData;

				for (const auto& callback : m_TriggerStayEvents)
				{
					callback(triggerStayEvent);
				}
			}
			break;
		case Physics::CollisionEventType::Exit:
			if (!m_TriggerExitEvents.empty())
			{
				Physics::TriggerExitEvent* triggerExitEvent = (Physics::TriggerExitEvent*)triggerEventData;

				for (const auto& callback : m_TriggerExitEvents)
				{
					callback(triggerExitEvent);
				}
			}
			break;
		}
	}

	void JoltPhysicsEngine::OnCollisionEnterEvent(std::function<void(Physics::CollisionEnterEvent*)>&& enterEventFunc)
	{
		m_CollisionEnterEvents.emplace_back(enterEventFunc);
	}

	void JoltPhysicsEngine::OnCollisionStayEvent(std::function<void(Physics::CollisionStayEvent*)>&& stayEventFunc)
	{
		m_CollisionStayEvents.emplace_back(stayEventFunc);
	}

	void JoltPhysicsEngine::OnCollisionExitEvent(std::function<void(Physics::CollisionExitEvent*)>&& exitEventFunc)
	{
		m_CollisionExitEvents.emplace_back(exitEventFunc);
	}

	void JoltPhysicsEngine::OnTriggerEnterEvent(std::function<void(Physics::TriggerEnterEvent*)>&& enterEventFunc)
	{
		m_TriggerEnterEvents.emplace_back(enterEventFunc);
	}

	void JoltPhysicsEngine::OnTriggerStayEvent(std::function<void(Physics::TriggerStayEvent*)>&& stayEventFunc)
	{
		m_TriggerStayEvents.emplace_back(stayEventFunc);
	}

	void JoltPhysicsEngine::OnTriggerExitEvent(std::function<void(Physics::TriggerExitEvent*)>&& exitEventFunc)
	{
		m_TriggerExitEvents.emplace_back(exitEventFunc);
	}

	void JoltPhysicsEngine::SetPosition(Component::Rigidbody& rb, const glm::vec3& position)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		bodyInterface.SetPosition(GetBodyIDFromPhysicsID(rb.BodyID), { position.x, position.y, position.z }, JPH::EActivation::Activate);
	}

	void JoltPhysicsEngine::SetRotation(Component::Rigidbody& rb, const glm::vec3& rotation)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		glm::quat qRot = glm::quat({ glm::radians(rotation.x), glm::radians(rotation.y), glm::radians(rotation.z) });
		bodyInterface.SetRotation(GetBodyIDFromPhysicsID(rb.BodyID), { qRot.x, qRot.y, qRot.z, qRot.w }, JPH::EActivation::Activate);
	}
	
	void JoltPhysicsEngine::AddLinearVelocity(Component::Rigidbody& rb, const glm::vec3& linearVelocity)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		bodyInterface.AddLinearVelocity(GetBodyIDFromPhysicsID(rb.BodyID), { linearVelocity.x, linearVelocity.y, linearVelocity.z });
	}

	void JoltPhysicsEngine::SetLinearVelocity(Component::Rigidbody& rb, const glm::vec3& linearVelocity)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		bodyInterface.SetLinearVelocity(GetBodyIDFromPhysicsID(rb.BodyID), { linearVelocity.x, linearVelocity.y, linearVelocity.z });
	}

	glm::vec3 JoltPhysicsEngine::GetLinearVelocity(Component::Rigidbody& rb)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		const auto& v = bodyInterface.GetLinearVelocity(GetBodyIDFromPhysicsID(rb.BodyID));
		return { v.GetX(), v.GetY(), v.GetZ() };
	}

	void JoltPhysicsEngine::SetAngularVelocity(Component::Rigidbody& rb, const glm::vec3& angularVelocity)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		bodyInterface.SetAngularVelocity(GetBodyIDFromPhysicsID(rb.BodyID), { angularVelocity.x, angularVelocity.y, angularVelocity.z });
	}

	glm::vec3 JoltPhysicsEngine::GetAngularVelocity(Component::Rigidbody& rb)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		const auto& v = bodyInterface.GetAngularVelocity(GetBodyIDFromPhysicsID(rb.BodyID));
		return { v.GetX(), v.GetY(), v.GetZ() };
	}

	void JoltPhysicsEngine::ApplyForce(Component::Rigidbody& rb, const glm::vec3& force)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		bodyInterface.AddForce(GetBodyIDFromPhysicsID(rb.BodyID), { force.x, force.y, force.z });
	}

	void JoltPhysicsEngine::ApplyTorque(Component::Rigidbody& rb, const glm::vec3& torque)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		bodyInterface.AddTorque(GetBodyIDFromPhysicsID(rb.BodyID), { torque.x, torque.y, torque.z });
	}

	void JoltPhysicsEngine::ApplyImpulse(Component::Rigidbody& rb, const glm::vec3& impluse)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		bodyInterface.AddImpulse(GetBodyIDFromPhysicsID(rb.BodyID), { impluse.x, impluse.y, impluse.z });
	}

	void JoltPhysicsEngine::ApplyAngularImpulse(Component::Rigidbody& rb, const glm::vec3& angularImpulse)
	{
		auto& bodyInterface = m_PhysicsSystem->GetBodyInterfaceNoLock();
		bodyInterface.AddAngularImpulse(GetBodyIDFromPhysicsID(rb.BodyID), { angularImpulse.x, angularImpulse.y, angularImpulse.z });
	}
}

