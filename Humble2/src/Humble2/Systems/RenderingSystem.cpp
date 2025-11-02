#include "RenderingSystem.h"

#include "Core/Application.h"
#include "Renderer/DebugRenderer.h"

namespace HBL2
{
	void RenderingSystem::OnCreate()
	{
		m_ResourceManager = ResourceManager::Instance;
		m_EditorScene = m_ResourceManager->GetScene(Context::EditorScene);

		// Switch on the renderer type.
		m_SceneRenderer = new ForwardSceneRenderer;

		m_SceneRenderer->Initialize(m_Context);
	}

	void RenderingSystem::OnUpdate(float ts)
	{
		BEGIN_PROFILE_SYSTEM();

		Entity mainCamera = GetMainCamera();

		void* frameData = m_SceneRenderer->Gather(mainCamera);

		// Renderer::Instance->CollectRenderData(frameData);

		m_SceneRenderer->Render(frameData);

		END_PROFILE_SYSTEM(RunningTime);
	}

	void RenderingSystem::OnDestroy()
	{
		m_SceneRenderer->CleanUp();

		delete m_SceneRenderer;
		m_SceneRenderer = nullptr;
	}

	Entity RenderingSystem::GetMainCamera()
	{
		if (Context::Mode == Mode::Runtime)
		{
			if (m_Context->MainCamera != Entity::Null)
			{
				return m_Context->MainCamera;
			}

			HBL2_CORE_WARN("No main camera set for runtime context.");
		}
		else if (Context::Mode == Mode::Editor)
		{
			if (m_EditorScene->MainCamera != Entity::Null)
			{
				return m_EditorScene->MainCamera;
			}

			HBL2_CORE_WARN("No main camera set for editor context.");
		}

		return Entity::Null;
	}
}
