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
			.debugName = "unlit-colored-layout",
			.bufferBindings = {
				{
					.slot = 0,
					.visibility = ShaderStage::VERTEX,
					.type = BufferBindingType::UNIFORM,
				},
			},
		});

		m_CameraBuffer = rm->CreateBuffer({
			.debugName = "camera-uniform-buffer",
			.usage = BufferUsage::UNIFORM,
			.usageHint = BufferUsageHint::DYNAMIC,
			.memory = Memory::GPU_CPU,
			.byteSize = 64,
			.initialData = nullptr
		});

		m_GlobalBindings = rm->CreateBindGroup({
			.debugName = "unlit-colored-bind-group",
			.layout = m_GlobalBindGroupLayout,
			.buffers = {
				{ .buffer = m_CameraBuffer },
			}
		});
	}

	void StaticMeshRenderingSystem::OnUpdate(float ts)
	{
		const glm::mat4& vp = GetViewProjection();
		
		m_UniformRingBuffer->Invalidate();

		CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::MAIN);
		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(Handle<RenderPass>(), Handle<FrameBuffer>());

		Renderer::Instance->SetBufferData(m_GlobalBindings, 0, (void*)&vp);

		DrawList draws;
		
		struct PerDrawData
		{
			glm::mat4 Model;
			glm::vec4 Color;
		};

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
					alloc.Data->Color = material->AlbedoColor;

					draws.Insert({
						.Shader = material->Shader,
						.BindGroup = material->BindGroup,
						.Mesh = staticMesh.Mesh,
						.Material = staticMesh.Material,
						.Offset = alloc.Offset,
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
	}

	const glm::mat4& StaticMeshRenderingSystem::GetViewProjection()
	{
		if (Context::Mode == Mode::Runtime)
		{
			if (m_Context->MainCamera != entt::null)
			{
				return m_Context->GetComponent<Component::Camera>(m_Context->MainCamera).ViewProjectionMatrix;
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
				return m_EditorScene->GetComponent<Component::Camera>(m_EditorScene->MainCamera).ViewProjectionMatrix;
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

		return glm::mat4(1.0f);
	}
}
