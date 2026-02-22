#include "Physics2dSystem.h"

#include "Core\Time.h"
#include "Project\Project.h"

namespace HBL2
{
	void Physics2dSystem::OnCreate()
	{
		// Retrieve project settings.
		const auto& projectSettings = Project::GetActive()->GetSpecification().Settings;

		// Switch on the physics engine implementation.
		switch (projectSettings.Physics2DImpl)
		{
		case Physics2DEngineImpl::BOX2D:
		case Physics2DEngineImpl::CUSTOM:
			PhysicsEngine2D::Instance = new Box2DPhysicsEngine;
			break;
		}

		// Apply physics settings.
		PhysicsEngine2D::Instance->SetDebugDrawEnabled(projectSettings.EnableDebugDraw2D);

		PhysicsEngine2D::Instance->Initialize(m_Context);
		m_Initialized = true;
	}

	void Physics2dSystem::OnFixedUpdate()
	{
		BEGIN_PROFILE_SYSTEM();

		// Update an progress the simulation.
		PhysicsEngine2D::Instance->Update();

		END_PROFILE_SYSTEM(RunningTime);
	}

	void Physics2dSystem::OnDestroy()
	{
		if (!m_Initialized)
		{
			return;
		}

		PhysicsEngine2D::Instance->Shutdown();

		m_Initialized = false;
	}

	
}
