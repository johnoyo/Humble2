#include "JoltPhysicsEngine.h"

namespace HBL2
{
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
}

