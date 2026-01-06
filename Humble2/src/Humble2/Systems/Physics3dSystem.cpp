#include "Physics3dSystem.h"

#include "Project/Project.h"

namespace HBL2
{
	void Physics3dSystem::OnCreate()
	{
		// Retrieve project settings.
		const auto& projectSettings = Project::GetActive()->GetSpecification().Settings;

		// Switch on the physics engine implementation.
		switch (projectSettings.Physics3DImpl)
		{
		case Physics3DEngineImpl::JOLT:
		case Physics3DEngineImpl::CUSTOM:
			PhysicsEngine3D::Instance = new JoltPhysicsEngine;
			break;
		}

		// Apply physics settings.
		PhysicsEngine3D::Instance->SetDebugDrawEnabled(projectSettings.EnableDebugDraw3D);
		PhysicsEngine3D::Instance->ShowColliders(projectSettings.ShowColliders3D);
		PhysicsEngine3D::Instance->ShowBoundingBoxes(projectSettings.ShowBoundingBoxes3D);

		PhysicsEngine3D::Instance->Initialize(m_Context);
		m_Initialized = true;
	}

	void Physics3dSystem::OnUpdate(float ts)
	{
	}

	void Physics3dSystem::OnFixedUpdate()
	{
		BEGIN_PROFILE_SYSTEM();

		PhysicsEngine3D::Instance->Update();

		END_PROFILE_SYSTEM(RunningTime);
	}

	void Physics3dSystem::OnDestroy()
	{
		if (!m_Initialized)
		{
			return;
		}

		PhysicsEngine3D::Instance->Shutdown();

		m_Initialized = false;
	}
}
