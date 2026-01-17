#include "RenderingSystem.h"

#include "Project\Project.h"
#include "Core\Application.h"
#include "Renderer\DebugRenderer.h"

namespace HBL2
{
	void RenderingSystem::OnCreate()
	{
		m_ResourceManager = ResourceManager::Instance;
		m_EditorScene = m_ResourceManager->GetScene(Context::EditorScene);

		// Retrieve project settings.
		const auto& projectSettings = Project::GetActive()->GetSpecification().Settings;

		// Switch on the renderer type.
		switch (projectSettings.Renderer)
		{
		case RendererType::Forward:
		case RendererType::ForwardPlus:
		case RendererType::Deferred:
		case RendererType::Custom:
			m_SceneRenderer = new ForwardSceneRenderer;
			break;
		}

		m_SceneRenderer->Initialize(m_Context);
	}

	void RenderingSystem::OnUpdate(float ts)
	{
		BEGIN_PROFILE_SYSTEM();

		Entity mainCamera = GetMainCamera();

		m_SceneRenderer->Gather(mainCamera);
		Renderer::Instance->CollectRenderData(m_SceneRenderer, m_SceneRenderer->GetRenderData());

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
