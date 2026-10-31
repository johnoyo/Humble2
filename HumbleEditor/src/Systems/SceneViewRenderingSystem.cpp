#include "SceneViewRenderingSystem.h"

#include "Project/Project.h"
#include "Core/Application.h"
#include "Renderer/DebugRenderer.h"
#include "Renderer/InvalidSceneRenderer.h"
#include "Renderer/ForwardSceneRenderer.h"
#include "Renderer/ForwardPlusSceneRenderer.h"
#include "Script/BuildEngine.h"

namespace HBL2::Editor
{
	void SceneViewRenderingSystem::OnCreate()
	{
		m_ResourceManager = ResourceManager::Instance;
		m_RuntimeScene = m_ResourceManager->GetScene(Context::ActiveScene);

		// Retrieve project settings.
		const auto& projectSettings = Project::GetActive()->GetSpecification().Settings;

		EventDispatcher::Get().Register<SceneChangeEvent>([this, maxLights = projectSettings.MaxLights](const HBL2::SceneChangeEvent& e)
		{
			m_RuntimeScene = m_ResourceManager->GetScene(e.NewScene);

			if (m_SceneRenderer->IsInitialized())
			{
				m_SceneRenderer->CleanUp();
			}
			m_SceneRenderer->Initialize(m_RuntimeScene, maxLights, true);
			Renderer::Instance->SceneViewDepthTexture = m_SceneRenderer->GetDepthTexture();
			Renderer::Instance->SceneViewTexture = m_SceneRenderer->GetTexture();
			Renderer::Instance->InvalidateSceneViewTexture();

			// NOTE (John): We de-register and re-register the resize event so that it will be invoked after
			// the resize callback inside the SceneRenderer::Initialize, so the SceneRenderer::m_TargetTexture 
			// and SceneRenderer::m_DepthTexture will be the new up to date ones.
			EventDispatcher::Get().Deregister<RendererResizedEvent>(m_RendererResizeEventHandle);

			m_RendererResizeEventHandle = EventDispatcher::Get().Register<RendererResizedEvent>([this](const RendererResizedEvent& e)
			{
				Renderer::Instance->SceneViewDepthTexture = m_SceneRenderer->GetDepthTexture();
				Renderer::Instance->SceneViewTexture = m_SceneRenderer->GetTexture();
				Renderer::Instance->InvalidateSceneViewTexture();

				DebugRenderer::Instance->RecreateFrameBuffer(e.Width, e.Height);
			});
		});

		// Switch on the renderer type.
		switch (projectSettings.Renderer)
		{
		case RendererType::FORWARD:
			m_SceneRenderer = new ForwardSceneRenderer;
			break;
		case RendererType::FORWARD_PLUS:
			m_SceneRenderer = new ForwardPlusSceneRenderer;
			break;
		case RendererType::CUSTOM:
			{
				Handle<Script> sceneRendererHandle = AssetManager::Instance->GetAsset<Script>(projectSettings.RendererUUID);
				if (sceneRendererHandle.IsValid())
				{
					Script* sceneRendererScript = ResourceManager::Instance->GetScript(sceneRendererHandle);
					m_SceneRenderer = BuildEngine::Instance->RegisterSceneRenderer(sceneRendererScript->Name);
				}

				break;
			}
		}

		if (m_SceneRenderer == nullptr)
		{
			// Placeholder invalid scene renderer (outputs pink).
			m_SceneRenderer = new InvalidSceneRenderer;
		}
	}

	void SceneViewRenderingSystem::OnUpdate(float ts)
	{
		BEGIN_PROFILE_SYSTEM();

		m_SceneRenderer->Gather(m_Context->MainCamera);

		END_PROFILE_SYSTEM(RunningTime);
	}

	void SceneViewRenderingSystem::OnRender(CommandBuffer* commandBuffer, const RendererFrameData* renderData)
	{
		m_SceneRenderer->Render(commandBuffer, renderData->RenderIndex);

		DebugRenderer::Instance->Render(commandBuffer, renderData->RenderIndex);

		// Transition the layout of the texture that the scene is rendered to, in order to be sampled in the shader.
		ResourceManager::Instance->TransitionTextureLayout(
			commandBuffer,
			m_SceneRenderer->GetTexture(),
			TextureLayout::RENDER_ATTACHMENT,
			TextureLayout::SHADER_READ_ONLY
		);
	}

	void SceneViewRenderingSystem::OnDestroy()
	{
		EventDispatcher::Get().Deregister<RendererResizedEvent>(m_RendererResizeEventHandle);

		m_SceneRenderer->CleanUp();

		delete m_SceneRenderer;
		m_SceneRenderer = nullptr;
	}
}
