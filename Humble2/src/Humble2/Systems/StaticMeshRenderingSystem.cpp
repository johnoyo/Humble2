#include "StaticMeshRenderingSystem.h"

namespace HBL2
{
	void StaticMeshRenderingSystem::OnCreate()
	{
		auto* rm = ResourceManager::Instance;

		m_EditorScene = rm->GetScene(Context::EditorScene);
		m_UniformRingBuffer = Renderer::Instance->TempUniformRingBuffer;

		m_Context->GetRegistry()
			.group<Component::StaticMesh_New>(entt::get<Component::Transform>)
			.each([&](Component::StaticMesh_New& staticMesh, Component::Transform& transform)
			{
				if (staticMesh.Enabled)
				{
					HBL2_CORE_INFO("Setting up mesh");
				}
			});

		m_GlobalBindGroupLayout = rm->CreateBindGroupLayout({
			.debugName = "global-bind-group-layout",
			.bufferBindings = {
				{
					.slot = 0,
					.visibility = ShaderStage::VERTEX,
					.type = BufferBindingType::UNIFORM,
				},
				{
					.slot = 1,
					.visibility = ShaderStage::FRAGMENT,
					.type = BufferBindingType::UNIFORM,
				},
			},
		});

		m_CameraBuffer = rm->CreateBuffer({
			.debugName = "camera-uniform-buffer",
			.usage = BufferUsage::UNIFORM,
			.usageHint = BufferUsageHint::DYNAMIC,
			.memory = Memory::GPU_CPU,
			.byteSize = sizeof(CameraData),
			.initialData = nullptr
		});

		m_LightBuffer = rm->CreateBuffer({
			.debugName = "light-uniform-buffer",
			.usage = BufferUsage::UNIFORM,
			.usageHint = BufferUsageHint::DYNAMIC,
			.memory = Memory::GPU_CPU,
			.byteSize = sizeof(LightData),
			.initialData = nullptr
		});

		m_GlobalBindings = rm->CreateBindGroup({
			.debugName = "global-bind-group",
			.layout = m_GlobalBindGroupLayout,
			.buffers = {
				{ .buffer = m_CameraBuffer },
				{ .buffer = m_LightBuffer },
			}
		});

		glm::vec3 lightPosition = glm::vec3(0.0f, 5.0f, 10.0f);

		glm::mat4 T = glm::translate(glm::mat4(1.0f), lightPosition);
		glm::quat R = glm::quat({ glm::radians(-30.0f), glm::radians(0.0f), glm::radians(0.0f) });
		glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));

		glm::mat4 Matrix = T * glm::toMat4(R) * S;

		m_LightData.LightCount = 1.0f;
		m_LightData.LightPositions[0] = Matrix * glm::vec4(lightPosition, 1.0f);
		m_LightData.LightColors[0] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		m_LightData.LightIntensities[0].x = 1.5f;
	}

	void StaticMeshRenderingSystem::OnUpdate(float ts)
	{
		GetViewProjection();
		
		m_UniformRingBuffer->Invalidate();

		CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::MAIN);
		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(Handle<RenderPass>(), Handle<FrameBuffer>());

		Renderer::Instance->SetBufferData(m_GlobalBindings, 0, (void*)&m_CameraData);
		Renderer::Instance->SetBufferData(m_GlobalBindings, 1, (void*)&m_LightData);

		DrawList draws;

		m_Context->GetRegistry()
			.group<Component::StaticMesh_New>(entt::get<Component::Transform>)
			.each([&](Component::StaticMesh_New& staticMesh, Component::Transform& transform)
			{
				if (staticMesh.Enabled)
				{
					if (!staticMesh.Material.IsValid() || !staticMesh.Mesh.IsValid())
					{
						return;
					}

					Material* material = ResourceManager::Instance->GetMaterial(staticMesh.Material);

					auto alloc = m_UniformRingBuffer->BumpAllocate<PerDrawData>();
					alloc.Data->Model = transform.WorldMatrix;
					alloc.Data->InverseModel = glm::transpose(glm::inverse(transform.WorldMatrix));
					alloc.Data->Color = material->AlbedoColor;
					alloc.Data->Metallicness = material->Metalicness;
					alloc.Data->Roughness = material->Roughness;

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

		GlobalDrawStream globalDrawStream = { .BindGroup = m_GlobalBindings };
		passRenderer->DrawSubPass(globalDrawStream, draws);
		commandBuffer->EndRenderPass(*passRenderer);
		commandBuffer->Submit();
	}

	void StaticMeshRenderingSystem::OnDestroy()
	{
		auto* rm = ResourceManager::Instance;

		rm->DeleteBindGroup(m_GlobalBindings);
		rm->DeleteBindGroupLayout(m_GlobalBindGroupLayout);
		rm->DeleteBuffer(m_CameraBuffer);
		rm->DeleteBuffer(m_LightBuffer);
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
