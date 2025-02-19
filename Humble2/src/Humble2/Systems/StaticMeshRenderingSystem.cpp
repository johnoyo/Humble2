#include "StaticMeshRenderingSystem.h"

#include "Core/Window.h"

namespace HBL2
{
	void StaticMeshRenderingSystem::OnCreate()
	{
		auto* rm = ResourceManager::Instance;

		m_EditorScene = rm->GetScene(Context::EditorScene);
		m_UniformRingBuffer = Renderer::Instance->TempUniformRingBuffer;

		m_Context->GetRegistry()
			.group<Component::StaticMesh>(entt::get<Component::Transform>)
			.each([&](Component::StaticMesh& staticMesh, Component::Transform& transform)
			{
				if (staticMesh.Enabled)
				{
					HBL2_CORE_INFO("Setting up mesh");
				}
			});

		Handle<RenderPassLayout> renderPassLayout = rm->CreateRenderPassLayout({
			.debugName = "main-renderpass-layout",
			.depthTargetFormat = Format::D32_FLOAT,
			.subPasses = {
				{ .depthTarget = true, .colorTargets = 1, },
			},
		});

		m_RenderPass = rm->CreateRenderPass({
			.debugName = "main-renderpass",
			.layout = renderPassLayout,
			.depthTarget = {
				.loadOp = LoadOperation::CLEAR,
				.storeOp = StoreOperation::STORE,
				.stencilLoadOp = LoadOperation::DONT_CARE,
				.stencilStoreOp = StoreOperation::DONT_CARE,
				.prevUsage = TextureLayout::UNDEFINED,
				.nextUsage = TextureLayout::DEPTH_STENCIL,
			},
			.colorTargets = {
				{
					.loadOp = LoadOperation::CLEAR,
					.storeOp = StoreOperation::STORE,
					.prevUsage = TextureLayout::UNDEFINED,
					.nextUsage = TextureLayout::RENDER_ATTACHMENT,
				},
			},
		});

		m_FrameBuffer = rm->CreateFrameBuffer({
			.debugName = "viewport",
			.width = Window::Instance->GetExtents().x,
			.height = Window::Instance->GetExtents().y,
			.renderPass = m_RenderPass,
			.depthTarget = Renderer::Instance->MainDepthTexture,
			.colorTargets = { Renderer::Instance->MainColorTexture },
		});

		Renderer::Instance->AddCallbackOnResize("Mesh-Resize-FrameBuffer", [this](uint32_t w, uint32_t h) { OnResize(w, h); });
	}

	void StaticMeshRenderingSystem::OnUpdate(float ts)
	{
		GetViewProjection();
		
		Handle<BindGroup> globalBindings = Renderer::Instance->GetGlobalBindings3D();

		CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::MAIN, RenderPassStage::Opaque);
		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_RenderPass, m_FrameBuffer);

		ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&m_CameraData);

		m_LightData.LightCount = 0;

		m_Context->GetRegistry()
			.group<Component::Light>(entt::get<Component::Transform>)
			.each([&](Component::Light& light, Component::Transform& transform)
			{
				if (light.Enabled)
				{
					m_LightData.LightPositions[(int)m_LightData.LightCount] = transform.LocalMatrix * glm::vec4(transform.Translation, 1.0f);
					m_LightData.LightIntensities[(int)m_LightData.LightCount].x = light.Intensity;
					m_LightData.LightColors[(int)m_LightData.LightCount] = glm::vec4(light.Color, 1.0f);
					m_LightData.LightCount++;
				}
			});

		ResourceManager::Instance->SetBufferData(globalBindings, 1, (void*)&m_LightData);

		DrawList draws;

		m_Context->GetRegistry()
			.group<Component::StaticMesh>(entt::get<Component::Transform>)
			.each([&](Component::StaticMesh& staticMesh, Component::Transform& transform)
			{
				if (staticMesh.Enabled)
				{
					if (!staticMesh.Material.IsValid() || !staticMesh.Mesh.IsValid())
					{
						return;
					}

					Material* material = ResourceManager::Instance->GetMaterial(staticMesh.Material);

					if (material == nullptr)
					{
						return;
					}

					auto alloc = m_UniformRingBuffer->BumpAllocate<PerDrawData>();
					alloc.Data->Model = transform.WorldMatrix;
					alloc.Data->InverseModel = glm::transpose(glm::inverse(transform.WorldMatrix));
					alloc.Data->Color = material->AlbedoColor;
					alloc.Data->Glossiness = material->Glossiness;

					draws.Insert({
						.Shader = material->Shader,
						.BindGroup = material->BindGroup,
						.Mesh = staticMesh.Mesh,
						.Material = staticMesh.Material,
						.Offset = alloc.Offset,
						.Size = sizeof(PerDrawData),
					});
				}
			});

		GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings };
		passRenderer->DrawSubPass(globalDrawStream, draws);
		commandBuffer->EndRenderPass(*passRenderer);
		commandBuffer->Submit();
	}

	void StaticMeshRenderingSystem::OnDestroy()
	{
		auto* rm = ResourceManager::Instance;
		rm->DeleteRenderPass(m_RenderPass);
		rm->DeleteFrameBuffer(m_FrameBuffer);

		Renderer::Instance->RemoveOnResizeCallback("Mesh-Resize-FrameBuffer");
	}

	void StaticMeshRenderingSystem::OnResize(uint32_t width, uint32_t height)
	{
		ResourceManager::Instance->DeleteFrameBuffer(m_FrameBuffer);

		m_FrameBuffer = ResourceManager::Instance->CreateFrameBuffer({
			.debugName = "viewport",
			.width = width,
			.height = height,
			.renderPass = m_RenderPass,
			.depthTarget = Renderer::Instance->MainDepthTexture,
			.colorTargets = { Renderer::Instance->MainColorTexture },
		});
	}

	void StaticMeshRenderingSystem::GetViewProjection()
	{
		if (Context::Mode == Mode::Runtime)
		{
			if (m_Context->MainCamera != entt::null)
			{
				m_CameraData.ViewProjection = m_Context->GetComponent<Component::Camera>(m_Context->MainCamera).ViewProjectionMatrix;
				Component::Transform& tr = m_Context->GetComponent<Component::Transform>(m_Context->MainCamera);
				m_LightData.ViewPosition = tr.WorldMatrix * glm::vec4(tr.Translation, 1.0f);
				return;
			}
			else
			{
				HBL2_CORE_WARN("No main camera set for runtime context.");
			}
		}
		else if (Context::Mode == Mode::Editor)
		{
			if (m_EditorScene->MainCamera != entt::null)
			{
				m_CameraData.ViewProjection = m_EditorScene->GetComponent<Component::Camera>(m_EditorScene->MainCamera).ViewProjectionMatrix;
				Component::Transform& tr = m_EditorScene->GetComponent<Component::Transform>(m_EditorScene->MainCamera);
				m_LightData.ViewPosition = tr.WorldMatrix * glm::vec4(tr.Translation, 1.0f);
				return;
			}
			else
			{
				HBL2_CORE_WARN("No main camera set for editor context.");
			}
		}
		else
		{
			HBL2_CORE_WARN("No mode set for current context.");
		}

		m_CameraData.ViewProjection = glm::mat4(1.0f);
		m_LightData.ViewPosition = glm::vec4(0.0f);
	}
}
