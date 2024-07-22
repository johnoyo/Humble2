#include "TestRenderingSystem.h"

namespace HBL2
{
	void TestRendererSystem::OnCreate()
	{
		m_Positions = new float[18] {
			-0.5, -0.5, 0.0, // 0 - Bottom left
			 0.5, -0.5, 0.0, // 1 - Bottom right
			 0.5,  0.5, 0.0, // 2 - Top right
			 0.5,  0.5, 0.0, // 2 - Top right
			-0.5,  0.5, 0.0, // 3 - Top left
			-0.5, -0.5, 0.0  // 0 - Bottom left
		};

		auto vsCode = ShaderUtilities::Get().Compile("assets/shaders/unlit.vs", ShaderStage::VERTEX);
		auto fsCode = ShaderUtilities::Get().Compile("assets/shaders/unlit.fs", ShaderStage::FRAGMENT);

		auto vsCode1 = ShaderUtilities::Get().Compile("assets/shaders/unlit-colored.vs", ShaderStage::VERTEX);
		auto fsCode1 = ShaderUtilities::Get().Compile("assets/shaders/unlit-colored.fs", ShaderStage::FRAGMENT);

		auto* rm = ResourceManager::Instance;

		auto shader = rm->CreateShader({
			.debugName = "test_shader",
			.VS {.code = vsCode1, .entryPoint = "main" },
			.FS {.code = fsCode1, .entryPoint = "main" },
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = 12,
						.attributes = {
							{ .byteOffset = 0, .format = VertexFormat::FLOAT32x3 }
						},
					}
				}
			}
		});

		auto bindGroupLayout = rm->CreateBindGroupLayout({
			.debugName = "unlit-colored-layout",
			.bufferBindings = {
				{
					.slot = 0,
					.visibility = ShaderStage::VERTEX,
					.type = BufferBindingType::UNIFORM,
				},
				{
					.slot = 1,
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

		auto objectBuffer = rm->CreateBuffer({
			.debugName = "object-uniform-buffer",
			.usage = BufferUsage::UNIFORM,
			.usageHint = BufferUsageHint::DYNAMIC,
			.memory = Memory::GPU_CPU,
			.byteSize = 16,
			.initialData = nullptr,
		});

		auto bindGroup = rm->CreateBindGroup({
			.debugName = "unlit-colored-bind-group",
			.layout = bindGroupLayout,
			.buffers = {
				{ .buffer = cameraBuffer },
				{ .buffer = objectBuffer },
			}
		});

		auto material = rm->CreateMaterial({
			.debugName = "test_material",
			.shader = shader,
			.bindGroup = bindGroup,
		});

		auto buffer = rm->CreateBuffer({
			.debugName = "test_quad_positions",
			.byteSize = sizeof(float) * 18,
			.initialData = m_Positions,
		});

		auto meshResource = rm->CreateMesh({
			.debugName = "quad_mesh",
			.vertexOffset = 0,
			.vertexCount = 6,
			.vertexBuffers = { buffer },
		});

		Context::ActiveScene->GetRegistry()
			.group<Component::StaticMesh_New>(entt::get<Component::Transform>)
			.each([&](Component::StaticMesh_New& mesh, Component::Transform& transform)
			{
				if (mesh.Enabled)
				{
					HBL2_CORE_INFO("Setting up mesh");

					mesh.MaterialInstance = material;
					mesh.MeshInstance = meshResource;
				}
			});
	}

	void TestRendererSystem::OnUpdate(float ts)
	{
		glm::mat4 vp = Context::ActiveScene->GetComponent<Component::Camera>(Context::ActiveScene->MainCamera).ViewProjectionMatrix;

		Context::ActiveScene->GetRegistry()
			.group<Component::StaticMesh_New>(entt::get<Component::Transform>)
			.each([&](Component::StaticMesh_New& mesh, Component::Transform& transform)
			{
				if (mesh.Enabled)
				{
					Renderer::Instance->SetPipeline(mesh.MaterialInstance);
					Renderer::Instance->SetBuffers(mesh.MeshInstance);
					Renderer::Instance->SetBindGroups(mesh.MaterialInstance);

					// TODO: Update uniforms
					// ...
					Material* openGLMaterial = ResourceManager::Instance->GetMaterial(mesh.MaterialInstance);

					glm::mat4 mvp = vp * transform.WorldMatrix;
					Renderer::Instance->WriteBuffer(openGLMaterial->BindGroup, 0, &mvp);

					glm::vec4 color = glm::vec4(1.0, 0.0, 0.0, 1.0);
					Renderer::Instance->WriteBuffer(openGLMaterial->BindGroup, 1, &color);

					Renderer::Instance->Draw(mesh.MeshInstance, mesh.MaterialInstance);
				}
			});
	}
}
