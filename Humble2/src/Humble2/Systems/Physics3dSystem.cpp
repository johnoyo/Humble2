#include "Physics3dSystem.h"

#include "Core/Time.h"

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

	void Physics3dSystem::OnCreate()
	{
		JPH::RegisterDefaultAllocator();
		JPH::Trace = TraceImpl;
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();

		m_TempAllocator = new JPH::TempAllocatorImpl(15_MB);
		m_JobSystem.Init(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

		const uint32_t cMaxBodies = 65536;
		const uint32_t cNumBodyMutexes = 0;
		const uint32_t cMaxBodyPairs = 65536;
		const uint32_t cMaxContactConstraints = 10240;

		m_PhysicsSystem.Init(cMaxBodies,
							cNumBodyMutexes,
							cMaxBodyPairs,
							cMaxContactConstraints,
							m_BroadPhaseLayerInterface,
							m_ObjectVsBroadPhaseLayerFilter,
							m_ObjectVsObjectLayerFilter);

	}

	void Physics3dSystem::OnUpdate(float ts)
	{
	}

	void Physics3dSystem::OnFixedUpdate()
	{
		const float cDeltaTime = Time::FixedTimeStep;

		uint32_t step = 0;
		// while (bodyInterface.IsActive(bodyID))
		{
			// Next step
			++step;

			// Output current position and velocity of the sphere
			// ...

			// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
			const int cCollisionSteps = 1;

			// Step the world
			m_PhysicsSystem.Update(cDeltaTime, cCollisionSteps, m_TempAllocator, &m_JobSystem);
		}
	}

	void Physics3dSystem::OnDestroy()
	{
		// Unregisters all types with the factory and cleans up the default material
		JPH::UnregisterTypes();

		// Destroy temp allocator
		delete m_TempAllocator;
		m_TempAllocator = nullptr;

		// Destroy the factory
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}
}
