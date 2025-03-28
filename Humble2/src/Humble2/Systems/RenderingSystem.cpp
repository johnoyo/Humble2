#include "RenderingSystem.h"

#include "Core\Window.h"
#include "Utilities\ShaderUtilities.h"

namespace HBL2
{
	void RenderingSystem::OnCreate()
	{
		m_ResourceManager = ResourceManager::Instance;
		m_EditorScene = m_ResourceManager->GetScene(Context::EditorScene);
		m_UniformRingBuffer = Renderer::Instance->TempUniformRingBuffer;

		m_RenderPassLayout = m_ResourceManager->CreateRenderPassLayout({
			.debugName = "main-renderpass-layout",
			.depthTargetFormat = Format::D32_FLOAT,
			.subPasses = {
				{ .depthTarget = true, .colorTargets = 1, },
			},
		});

		MeshRenderingSetup();
		SpriteRenderingSetup();
		FullScreenQuadSetup();
	}

	void RenderingSystem::OnUpdate(float ts)
	{
		GetViewProjection();

		StaticMeshPass();
		SpritePass();
		PostProcessPass();
		CompositePass();
	}

	void RenderingSystem::OnDestroy()
	{
		m_ResourceManager->DeleteRenderPassLayout(m_RenderPassLayout);

		m_ResourceManager->DeleteRenderPass(m_MeshRenderPass);
		m_ResourceManager->DeleteFrameBuffer(m_MeshFrameBuffer);
		Renderer::Instance->RemoveOnResizeCallback("Mesh-Resize-FrameBuffer");

		m_ResourceManager->DeleteBuffer(m_VertexBuffer);
		m_ResourceManager->DeleteMesh(m_SpriteMesh);
		m_ResourceManager->DeleteRenderPass(m_SpriteRenderPass);
		m_ResourceManager->DeleteFrameBuffer(m_SpriteFrameBuffer);
		Renderer::Instance->RemoveOnResizeCallback("Sprite-Resize-FrameBuffer");

		m_ResourceManager->DeleteBuffer(m_QuadVertexBuffer);
		m_ResourceManager->DeleteMesh(m_QuadMesh);
		m_ResourceManager->DeleteMaterial(m_QuadMaterial);
	}

	void RenderingSystem::MeshRenderingSetup()
	{
		m_MeshRenderPass = m_ResourceManager->CreateRenderPass({
			.debugName = "mesh-renderpass",
			.layout = m_RenderPassLayout,
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

		m_MeshFrameBuffer = m_ResourceManager->CreateFrameBuffer({
			.debugName = "viewport-fb",
			.width = Window::Instance->GetExtents().x,
			.height = Window::Instance->GetExtents().y,
			.renderPass = m_MeshRenderPass,
			.depthTarget = Renderer::Instance->MainDepthTexture,
			.colorTargets = { Renderer::Instance->MainColorTexture },
		});

		Renderer::Instance->AddCallbackOnResize("Mesh-Resize-FrameBuffer", [this](uint32_t width, uint32_t height)
		{
			ResourceManager::Instance->DeleteFrameBuffer(m_MeshFrameBuffer);

			m_MeshFrameBuffer = ResourceManager::Instance->CreateFrameBuffer({
				.debugName = "viewport-fb",
				.width = width,
				.height = height,
				.renderPass = m_MeshRenderPass,
				.depthTarget = Renderer::Instance->MainDepthTexture,
				.colorTargets = { Renderer::Instance->MainColorTexture },
			});
		});
	}

	void RenderingSystem::SpriteRenderingSetup()
	{
		float* vertexBuffer = new float[30] {
			-0.5, -0.5, 0.0, 0.0, 1.0, // 0 - Bottom left
			 0.5, -0.5, 0.0, 1.0, 1.0, // 1 - Bottom right
			 0.5,  0.5, 0.0, 1.0, 0.0, // 2 - Top right
			 0.5,  0.5, 0.0, 1.0, 0.0, // 2 - Top right
			-0.5,  0.5, 0.0, 0.0, 0.0, // 3 - Top left
			-0.5, -0.5, 0.0, 0.0, 1.0, // 0 - Bottom left
		};

		m_VertexBuffer = m_ResourceManager->CreateBuffer({
			.debugName = "quad-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.byteSize = sizeof(float) * 30,
			.initialData = vertexBuffer,
		});

		m_SpriteMesh = m_ResourceManager->CreateMesh({
			.debugName = "quad-mesh",
			.meshes = {
				{
					.debugName = "quad-sub-mesh",
					.subMeshes = {
						{
							.vertexOffset = 0,
							.vertexCount = 6,
						}
					},
					.vertexBuffers = { m_VertexBuffer },
				}
			}
		});

		m_SpriteRenderPass = m_ResourceManager->CreateRenderPass({
			.debugName = "sprite-renderpass",
			.layout = m_RenderPassLayout,
			.depthTarget = {
				.loadOp = LoadOperation::LOAD,
				.storeOp = StoreOperation::STORE,
				.stencilLoadOp = LoadOperation::DONT_CARE,
				.stencilStoreOp = StoreOperation::DONT_CARE,
				.prevUsage = TextureLayout::DEPTH_STENCIL,
				.nextUsage = TextureLayout::DEPTH_STENCIL,
			},
			.colorTargets = {
				{
					.loadOp = LoadOperation::LOAD,
					.storeOp = StoreOperation::STORE,
					.prevUsage = TextureLayout::RENDER_ATTACHMENT,
					.nextUsage = TextureLayout::SHADER_READ_ONLY,
				},
			},
		});

		m_SpriteFrameBuffer = m_ResourceManager->CreateFrameBuffer({
			.debugName = "viewport-fb",
			.width = Window::Instance->GetExtents().x,
			.height = Window::Instance->GetExtents().y,
			.renderPass = m_SpriteRenderPass,
			.depthTarget = Renderer::Instance->MainDepthTexture,
			.colorTargets = { Renderer::Instance->MainColorTexture },
		});

		Renderer::Instance->AddCallbackOnResize("Sprite-Resize-FrameBuffer", [this](uint32_t width, uint32_t height)
		{
			ResourceManager::Instance->DeleteFrameBuffer(m_SpriteFrameBuffer);

			m_SpriteFrameBuffer = ResourceManager::Instance->CreateFrameBuffer({
				.debugName = "viewport-fb",
				.width = width,
				.height = height,
				.renderPass = m_SpriteRenderPass,
				.depthTarget = Renderer::Instance->MainDepthTexture,
				.colorTargets = { Renderer::Instance->MainColorTexture },
			});
		});
	}

	void RenderingSystem::FullScreenQuadSetup()
	{
		float* vertexBuffer = nullptr;

		if (Renderer::Instance->GetAPI() == GraphicsAPI::VULKAN)
		{
			vertexBuffer = new float[24] {
				-1.0, -1.0, 0.0, 1.0, // 0 - Bottom left
				 1.0, -1.0, 1.0, 1.0, // 1 - Bottom right
				 1.0,  1.0, 1.0, 0.0, // 2 - Top right
				 1.0,  1.0, 1.0, 0.0, // 2 - Top right
				-1.0,  1.0, 0.0, 0.0, // 3 - Top left
				-1.0, -1.0, 0.0, 1.0, // 0 - Bottom left
			};
		}
		else if (Renderer::Instance->GetAPI() == GraphicsAPI::OPENGL)
		{
			vertexBuffer = new float[24] {
				-1.0, -1.0, 0.0, 0.0, // 0 - Bottom left
				 1.0, -1.0, 1.0, 0.0, // 1 - Bottom right
				 1.0,  1.0, 1.0, 1.0, // 2 - Top right
				 1.0,  1.0, 1.0, 1.0, // 2 - Top right
				-1.0,  1.0, 0.0, 1.0, // 3 - Top left
				-1.0, -1.0, 0.0, 0.0, // 0 - Bottom left
			};
		}

		m_QuadVertexBuffer = m_ResourceManager->CreateBuffer({
			.debugName = "quad-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.byteSize = sizeof(float) * 24,
			.initialData = vertexBuffer,
		});

		m_QuadMesh = m_ResourceManager->CreateMesh({
			.debugName = "quad_mesh",
			.meshes = {
				{
					.debugName = "",
					.subMeshes = {
						{
							.vertexOffset = 0,
							.vertexCount = 6,
						}
					},
					.vertexBuffers = { m_QuadVertexBuffer },
				}
			}
		});

		m_QuadMaterial = m_ResourceManager->CreateMaterial({
			.debugName = "fullscreen-quad-material",
			.shader = ShaderUtilities::Get().GetBuiltInShader(BuiltInShader::PRESENT),
		});
	}

	bool RenderingSystem::IsInFrustum(const Component::Transform& transform)
	{
		glm::vec3 position = transform.Translation;
		float radius = glm::length(transform.Scale) * 0.5f;

		for (const auto& plane : m_CameraFrustum.Planes)
		{
			float distance = glm::dot(plane.normal, position) + plane.distance;

			// If the sphere is completely outside the frustum, reject it
			if (distance < -radius)
			{
				return false; // Outside
			}
		}

		return true; // Inside or intersecting
	}

	bool RenderingSystem::IsInFrustum(Handle<Mesh> meshHandle, const Component::Transform& transform)
	{
		Mesh* mesh = ResourceManager::Instance->GetMesh(meshHandle);

		if (mesh == nullptr)
		{
			return false;
		}

		// Transform AABB min/max to world space
		glm::vec3 worldMin = glm::vec3(transform.WorldMatrix * glm::vec4(mesh->Extents.Min.x, mesh->Extents.Min.y, mesh->Extents.Min.z, 1.0f));
		glm::vec3 worldMax = glm::vec3(transform.WorldMatrix * glm::vec4(mesh->Extents.Max.x, mesh->Extents.Max.y, mesh->Extents.Max.z, 1.0f));

		// Generate 8 corners of AABB
		std::array<glm::vec3, 8> corners = {
			glm::vec3(worldMin.x, worldMin.y, worldMin.z),
			glm::vec3(worldMax.x, worldMin.y, worldMin.z),
			glm::vec3(worldMin.x, worldMax.y, worldMin.z),
			glm::vec3(worldMax.x, worldMax.y, worldMin.z),
			glm::vec3(worldMin.x, worldMin.y, worldMax.z),
			glm::vec3(worldMax.x, worldMin.y, worldMax.z),
			glm::vec3(worldMin.x, worldMax.y, worldMax.z),
			glm::vec3(worldMax.x, worldMax.y, worldMax.z),
		};

		// Check if any of the 8 corners are inside the frustum
		for (const auto& plane : m_CameraFrustum.Planes)
		{
			bool inside = false;
			for (const glm::vec3& corner : corners)
			{
				if (glm::dot(plane.normal, corner) + plane.distance >= 0)
				{
					inside = true;
					break;  // If at least one point is inside, we continue
				}
			}

			if (!inside)
			{
				return false;  // If all points are outside any plane, it's not in frustum
			}
		}

		return true;  // If we pass all planes, the object is inside the frustum
	}

	void RenderingSystem::StaticMeshPass()
	{
		Handle<BindGroup> globalBindings = Renderer::Instance->GetGlobalBindings3D();

		CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::MAIN, RenderPassStage::Opaque);
		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_MeshRenderPass, m_MeshFrameBuffer);

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

		// Store the offset that the static meshes start from in the dynamic uniform buffer.
		uint32_t offset = m_UniformRingBuffer->GetCurrentOffset();

		DrawList draws;

		m_Context->GetRegistry()
			.group<Component::StaticMesh>(entt::get<Component::Transform>)
			.each([&](Component::StaticMesh& staticMesh, Component::Transform& transform)
			{
				if (staticMesh.Enabled)
				{
					if (!IsInFrustum(staticMesh.Mesh, transform))
					{
						return;
					}

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
						.MeshIndex = staticMesh.MeshIndex,
						.SubMeshIndex = staticMesh.SubMeshIndex,
						.Material = staticMesh.Material,
						.Offset = alloc.Offset,
						.Size = sizeof(PerDrawData),
					});
				}
			});

		// Store the total size of the static mesh draw data in the dynamic uniform data.
		uint32_t size = draws.GetCount() * m_UniformRingBuffer->GetAlignedSize(sizeof(PerDrawData));

		GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings, .DynamicUniformBufferOffset = offset, .DynamicUniformBufferSize = size };
		passRenderer->DrawSubPass(globalDrawStream, draws);
		commandBuffer->EndRenderPass(*passRenderer);
		commandBuffer->Submit();
	}

	void RenderingSystem::SpritePass()
	{
		const glm::mat4& vp = m_CameraData.ViewProjection;

		Handle<BindGroup> globalBindings = Renderer::Instance->GetGlobalBindings2D();

		CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::MAIN, RenderPassStage::OpaqueSprite);
		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_SpriteRenderPass, m_SpriteFrameBuffer);

		ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&vp);

		// Store the offset that the sprites start from in the dynamic uniform buffer.
		uint32_t offset = m_UniformRingBuffer->GetCurrentOffset();

		DrawList draws;

		m_Context->GetRegistry()
			.group<Component::Sprite>(entt::get<Component::Transform>)
			.each([&](Component::Sprite& sprite, Component::Transform& transform)
			{
				if (sprite.Enabled)
				{
					if (!IsInFrustum(transform))
					{
						return;
					}

					if (!sprite.Material.IsValid())
					{
						return;
					}

					Material* material = ResourceManager::Instance->GetMaterial(sprite.Material);

					if (material == nullptr)
					{
						return;
					}

					auto alloc = m_UniformRingBuffer->BumpAllocate<PerDrawDataSprite>();
					alloc.Data->Model = transform.WorldMatrix;
					alloc.Data->Color = material->AlbedoColor;

					draws.Insert({
						.Shader = material->Shader,
						.BindGroup = material->BindGroup,
						.Mesh = m_SpriteMesh,
						.Material = sprite.Material,
						.Offset = alloc.Offset,
						.Size = sizeof(PerDrawDataSprite),
					});
				}
			});

		// Store the total size of the sprite draw data in the dynamic uniform data.
		uint32_t size = draws.GetCount() * m_UniformRingBuffer->GetAlignedSize(sizeof(PerDrawDataSprite));

		GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings, .DynamicUniformBufferOffset = offset, .DynamicUniformBufferSize = size };
		passRenderer->DrawSubPass(globalDrawStream, draws);
		commandBuffer->EndRenderPass(*passRenderer);
		commandBuffer->Submit();
	}

	void RenderingSystem::PostProcessPass()
	{
		// Post Process Pass
		/*CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::MAIN, RenderPassStage::PostProcess);
		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_RenderPass, m_FrameBuffer);

		commandBuffer->EndRenderPass(*passRenderer);
		commandBuffer->Submit();*/
	}

	void RenderingSystem::CompositePass()
	{
		CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::MAIN, RenderPassStage::Present);
		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(Renderer::Instance->GetMainRenderPass(), Renderer::Instance->GetMainFrameBuffer());

		DrawList draws;
		draws.Insert({
			.Shader = ShaderUtilities::Get().GetBuiltInShader(BuiltInShader::PRESENT),
			.Mesh = m_QuadMesh,
			.Material = m_QuadMaterial,
		});

		GlobalDrawStream globalDrawStream = { .BindGroup = Renderer::Instance->GetGlobalPresentBindings() };
		passRenderer->DrawSubPass(globalDrawStream, draws);
		commandBuffer->EndRenderPass(*passRenderer);
		commandBuffer->Submit();
	}

	void RenderingSystem::GetViewProjection()
	{
		if (Context::Mode == Mode::Runtime)
		{
			if (m_Context->MainCamera != entt::null)
			{
				Component::Camera& camera = m_Context->GetComponent<Component::Camera>(m_Context->MainCamera);
				m_CameraData.ViewProjection = camera.ViewProjectionMatrix;
				m_CameraFrustum = camera.Frustum;
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
				Component::Camera& camera = m_EditorScene->GetComponent<Component::Camera>(m_EditorScene->MainCamera);
				m_CameraData.ViewProjection = camera.ViewProjectionMatrix;
				m_CameraFrustum = camera.Frustum;
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
		m_CameraFrustum = {};
	}
}

/*
DrawList& draws = GetDrawList3D();

auto& renderPasses = Renderer::Instance->GetRenderPassess3D();

renderPasses.Execute(RenderPassEvent::BeforeRendering);

renderPasses.Execute(RenderPassEvent::BeforeRenderingShadows);
ShadowPass();
renderPasses.Execute(RenderPassEvent::AfterRenderingShadows);

renderPasses.Execute(RenderPassEvent::BeforeRenderingOpaques);
OpaqueGeometryPass();
renderPasses.Execute(RenderPassEvent::AfterRenderingOpaques);

renderPasses.Execute(RenderPassEvent::BeforeRenderingSkybox);
SkyboxPass();
renderPasses.Execute(RenderPassEvent::AfterRenderingSkybox);

renderPasses.Execute(RenderPassEvent::BeforeRenderingTransparents);
TransparentGeometryPass();
renderPasses.Execute(RenderPassEvent::AfterRenderingTransparents);

renderPasses.Execute(RenderPassEvent::BeforeRenderingPostProcess);
PostProcessPass();
renderPasses.Execute(RenderPassEvent::AfterRenderingPostProcess);

renderPasses.Execute(RenderPassEvent::AfterRendering);

class RenderPassPool
{
public:
	void AddRenderPass(ScriptableRenderPass* renderPass);
	void SetUp(RenderPassEvent event);
	void Execute(RenderPassEvent event)
	{
		for (const auto& renderPass : renderPasses)
		{
			if (renderPass->GetInjectionPoint() == event)
			{
				renderPass->Excecute();
			}
		}
	}
	void CleanUp(RenderPassEvent event);

private:
	std::vector<ScriptableRenderPass*> m_RenderPasses;
};

class ScriptableRenderPass
{
public:
	virtual void SetUp() = 0;
	virtual void Execute() = 0;
	virtual void CleanUp() = 0;

	const RenderPassEvent& GetInjectionPoint() const { return m_InjectionPoint; }

protected:
	RenderPassEvent m_InjectionPoint;
	const char* m_PassName;
};

class DitherRenderPass : public ScriptableRenderPass
{
public:
	virtual void SetUp() override
	{
		m_PassName = "DitherRenderPass";
		m_InjectionPoint = RenderPassEvent::AfterRenderingPostProcess;
	}

	virtual void Execute() override
	{
		CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::CUSTOM, RenderPassStage::PostProcess);
		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(Renderer::Instance->GetMainRenderPass(), Renderer::Instance->GetMainFrameBuffer());

		GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings };
		passRenderer->DrawSubPass(globalDrawStream, draws);
		commandBuffer->EndRenderPass(*passRenderer);
		commandBuffer->Submit();
	}

	virtual void CleanUp() override
	{
	}
};
*/
