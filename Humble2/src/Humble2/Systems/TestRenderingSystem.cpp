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

		auto* rm = ResourceManager::Instance;

		Context::ActiveScene->GetRegistry()
			.group<Component::StaticMesh_New>(entt::get<Component::Transform>)
			.each([&](Component::StaticMesh_New& mesh, Component::Transform& transform)
			{
				if (mesh.Enabled)
				{
					HBL2_CORE_INFO("Setting up mesh");

					auto shader = rm->CreateShader({
						.debugName = "test_shader",
						.VS { .code = "assets/shaders/unlit.vs", .entryPoint = "main" },
						.FS { .code = "assets/shaders/unlit.fs", .entryPoint = "main" },
						.renderPipeline {
							.vertexBufferBindings = {
								{
									.byteStride = 12,
									.attributes = {
										{ .byteOffset = 0, .format = 484758 }
									},
								}
							}
						}
					});

					auto material = rm->CreateMaterial({
						.debugName = "test_material",
						.shader = shader,
					});

					mesh.MaterialInstance = material;
					
					auto buffer = rm->CreateBuffer({
						.debugName = "test_quad_positions",
						.initialData = m_Positions,
						.byteSize = sizeof(float) * 18,
					});

					auto meshResource = rm->CreateMesh({
						.debugName = "quad_mesh",
						.vertexOffset = 0,
						.vertexCount = 6,
						.vertexBuffers = { buffer },
					});

					mesh.MeshInstance = meshResource;
				}
			});
	}

	void TestRendererSystem::OnUpdate(float ts)
	{
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

					Renderer::Instance->Draw(mesh.MeshInstance, mesh.MaterialInstance);
				}
			});
	}
}
