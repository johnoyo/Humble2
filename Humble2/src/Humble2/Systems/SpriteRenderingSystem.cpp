#include "SpriteRenderingSystem.h"

namespace HBL2
{
	void SpriteRenderingSystem::OnCreate()
	{
		auto* rm = ResourceManager::Instance;

		m_EditorScene = rm->GetScene(Context::EditorScene);
		m_UniformRingBuffer = Renderer::Instance->TempUniformRingBuffer;

		float* vertexBuffer = new float[30] {
			-0.5, -0.5, 0.0, 0.0, 1.0, // 0 - Bottom left
			 0.5, -0.5, 0.0, 1.0, 1.0, // 1 - Bottom right
			 0.5,  0.5, 0.0, 1.0, 0.0, // 2 - Top right
			 0.5,  0.5, 0.0, 1.0, 0.0, // 2 - Top right
			-0.5,  0.5, 0.0, 0.0, 0.0, // 3 - Top left
			-0.5, -0.5, 0.0, 0.0, 1.0, // 0 - Bottom left
		};

		m_VertexBuffer = rm->CreateBuffer({
			.debugName = "quad_vertex_buffer",
			.byteSize = sizeof(float) * 30,
			.initialData = vertexBuffer,
		});

		m_SpriteMesh = rm->CreateMesh({
			.debugName = "quad_mesh",
			.vertexOffset = 0,
			.vertexCount = 6,
			.vertexBuffers = { m_VertexBuffer },
		});

		m_Context->GetRegistry()
			.group<Component::Sprite_New>(entt::get<Component::Transform>)
			.each([&](Component::Sprite_New& sprite, Component::Transform& transform)
			{
				if (sprite.Enabled)
				{
					HBL2_CORE_INFO("Setting up sprite");
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

	void SpriteRenderingSystem::OnUpdate(float ts)
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
			.group<Component::Sprite_New>(entt::get<Component::Transform>)
			.each([&](Component::Sprite_New& sprite, Component::Transform& transform)
			{
				if (sprite.Enabled)
				{
					if (!sprite.Material.IsValid())
					{
						return;
					}

					Material* material = ResourceManager::Instance->GetMaterial(sprite.Material);

					auto alloc = m_UniformRingBuffer->BumpAllocate<PerDrawData>();
					alloc.Data->Model = transform.WorldMatrix;
					alloc.Data->Color = material->AlbedoColor;

					draws.Insert({
						.Shader = material->Shader,
						.BindGroup = material->BindGroup,
						.Mesh = m_SpriteMesh,
						.Material = sprite.Material,
						.Offset = alloc.Offset,
					});
				}
			});

		GlobalDrawStream globalDrawStream = { .BindGroup = m_GlobalBindings };
		passRenderer->DrawSubPass(globalDrawStream, draws);
		commandBuffer->EndRenderPass(*passRenderer);
		commandBuffer->Submit();
	}

	void SpriteRenderingSystem::OnDestroy()
	{
		auto* rm = ResourceManager::Instance;

		rm->DeleteBindGroup(m_GlobalBindings);
		rm->DeleteBindGroupLayout(m_GlobalBindGroupLayout);
		rm->DeleteBuffer(m_CameraBuffer);
		rm->DeleteBuffer(m_VertexBuffer);
		rm->DeleteMesh(m_SpriteMesh);
	}

	const glm::mat4& SpriteRenderingSystem::GetViewProjection()
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
