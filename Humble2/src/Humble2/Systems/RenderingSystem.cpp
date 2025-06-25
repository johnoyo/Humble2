#include "RenderingSystem.h"

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
		entt::entity mainCamera = GetMainCamera();

		m_SceneRenderer->Render(mainCamera);
	}

	void RenderingSystem::OnDestroy()
	{
		m_SceneRenderer->CleanUp();
	}

	entt::entity RenderingSystem::GetMainCamera()
	{
		if (Context::Mode == Mode::Runtime)
		{
			if (m_Context->MainCamera != entt::null)
			{
				return m_Context->MainCamera;
			}

			HBL2_CORE_WARN("No main camera set for runtime context.");
		}
		else if (Context::Mode == Mode::Editor)
		{
			if (m_EditorScene->MainCamera != entt::null)
			{
				return m_EditorScene->MainCamera;
			}

			HBL2_CORE_WARN("No main camera set for editor context.");
		}

		return entt::null;
	}
}
