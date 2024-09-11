#include "StaticMeshRenderingSystem.h"

namespace HBL2
{
	void StaticMeshRenderingSystem::OnCreate()
	{
		m_UniformRingBuffer = Renderer::Instance->TempUniformRingBuffer;

		float* positions = new float[18] {
			-0.5, -0.5, 0.0, // 0 - Bottom left
			 0.5, -0.5, 0.0, // 1 - Bottom right
			 0.5,  0.5, 0.0, // 2 - Top right
			 0.5,  0.5, 0.0, // 2 - Top right
			-0.5,  0.5, 0.0, // 3 - Top left
			-0.5, -0.5, 0.0  // 0 - Bottom left
		};

		float* texCoords = new float[12] {
			0.0, 1.0,  // 0 - Bottom left
			1.0, 1.0,  // 1 - Bottom right
			1.0, 0.0,  // 2 - Top right
			1.0, 0.0,  // 2 - Top right
			0.0, 0.0,  // 3 - Top left
			0.0, 1.0,  // 0 - Bottom left
		};

		auto* rm = ResourceManager::Instance;

		//TextureSettings textureSettings;
		//auto textureData = TextureUtilities::Get().Load("assets/icons/content_browser/png-1477.png", textureSettings);

		//auto shaderCode = ShaderUtilities::Get().Compile("assets/shaders/unlit-textured.hblshader");
		//auto shaderCode = ShaderUtilities::Get().Compile("assets/shaders/unlit-colored.hblshader");

		//auto shader = rm->CreateShader({
		//	.debugName = "test_shader",
		//	.VS { .code = shaderCode[0], .entryPoint = "main" },
		//	.FS { .code = shaderCode[1], .entryPoint = "main" },
		//	.renderPipeline {
		//		.vertexBufferBindings = {
		//			{
		//				.byteStride = 12,
		//				.attributes = {
		//					{ .byteOffset = 0, .format = VertexFormat::FLOAT32x3 },
		//				},
		//			}/*,
		//			{
		//				.byteStride = 8,
		//				.attributes = {
		//					{ .byteOffset = 0, .format = VertexFormat::FLOAT32x2 },
		//				},
		//			}*/
		//		}
		//	}
		//});

		//auto drawBindGroupLayout = rm->CreateBindGroupLayout({
		//	.debugName = "unlit-colored-layout",
		//	/*.textureBindings = {
		//		{
		//			.slot = 0,
		//			.visibility = ShaderStage::FRAGMENT,
		//		}
		//	},*/
		//	.bufferBindings = {
		//		{
		//			.slot = 1,
		//			.visibility = ShaderStage::VERTEX,
		//			.type = BufferBindingType::UNIFORM_DYNAMIC_OFFSET,
		//		},
		//	},			
		//});

		///*auto texture = rm->CreateTexture({
		//	.debugName = "test-texture",
		//	.dimensions = { textureData.Width, textureData.Height, 0 },
		//	.initialData = textureData.Data,
		//});*/

		//m_DrawBindings = rm->CreateBindGroup({
		//	.debugName = "unlit-colored-bind-group",
		//	.layout = drawBindGroupLayout,
		//	/*.textures = { texture },*/
		//	.buffers = {
		//		{ .buffer = m_UniformRingBuffer->GetBuffer() },
		//	}
		//});

		//auto material = rm->CreateMaterial({
		//	.debugName = "test_material",
		//	.shader = shader,
		//	.bindGroup = m_DrawBindings,
		//});

		auto buffer = rm->CreateBuffer({
			.debugName = "test_quad_positions",
			.byteSize = sizeof(float) * 18,
			.initialData = positions,
		});

		//auto bufferTexCoords = rm->CreateBuffer({
		//	.debugName = "test_quad_texCoords",
		//	.byteSize = sizeof(float) * 12,
		//	.initialData = texCoords,
		//});

		auto meshResource = rm->CreateMesh({
			.debugName = "quad_mesh",
			.vertexOffset = 0,
			.vertexCount = 6,
			.vertexBuffers = { buffer/*, bufferTexCoords*/ },
		});

		m_Context->GetRegistry()
			.group<Component::StaticMesh_New>(entt::get<Component::Transform>)
			.each([&](Component::StaticMesh_New& staticMesh, Component::Transform& transform)
			{
				if (staticMesh.Enabled)
				{
					HBL2_CORE_INFO("Setting up mesh");

					//staticMesh.Material = material;
					staticMesh.Mesh = meshResource;
				}
			});

		auto globalBindGroupLayout = rm->CreateBindGroupLayout({
			.debugName = "unlit-colored-layout",
			.bufferBindings = {
				{
					.slot = 0,
					.visibility = ShaderStage::VERTEX,
					.type = BufferBindingType::UNIFORM,
				},
			},
		});

		auto cameraBuffer = rm->CreateBuffer({
			.debugName = "camera-uniform-buffer",
			.usage = BufferUsage::UNIFORM,
			.usageHint = BufferUsageHint::DYNAMIC,
			.memory = Memory::GPU_CPU,
			.byteSize = 64,
			.initialData = nullptr
		});

		m_GlobalBindings = rm->CreateBindGroup({
			.debugName = "unlit-colored-bind-group",
			.layout = globalBindGroupLayout,
			.buffers = {
				{ .buffer = cameraBuffer },
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
					alloc.Data->Color = glm::vec4(1.0, 1.0, 0.75, 1.0);

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
	}

	const glm::mat4& StaticMeshRenderingSystem::GetViewProjection()
	{
		if (Context::Mode == Mode::Runtime)
		{
			if (Context::ActiveScene->MainCamera != entt::null)
			{
				return Context::ActiveScene->GetComponent<Component::Camera>(Context::ActiveScene->MainCamera).ViewProjectionMatrix;
			}
			else
			{
				HBL2_CORE_WARN("No main camera set for runtime context.");
			}
		}
		else if (Context::Mode == Mode::Editor)
		{
			if (Context::EditorScene->MainCamera != entt::null)
			{
				return Context::EditorScene->GetComponent<Component::Camera>(Context::EditorScene->MainCamera).ViewProjectionMatrix;
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
