#include "ForwardRenderingSystem.h"

#include "Core\Window.h"
#include "Utilities\ShaderUtilities.h"

#include <glm/gtx/euler_angles.hpp>

namespace HBL2
{
	struct Attenuation
	{
		float distance;
		float constant;
		float linear;
		float quadratic;
	};

	static const std::vector<Attenuation> g_AttenuationTable =
	{
		{7,     1.0f, 0.7f,   1.8f},
		{13,    1.0f, 0.35f,  0.44f},
		{20,    1.0f, 0.22f,  0.20f},
		{32,    1.0f, 0.14f,  0.07f},
		{50,    1.0f, 0.09f,  0.032f},
		{65,    1.0f, 0.07f,  0.017f},
		{100,   1.0f, 0.045f, 0.0075f},
		{160,   1.0f, 0.027f, 0.0028f},
		{200,   1.0f, 0.022f, 0.0019f},
		{325,   1.0f, 0.014f, 0.0007f},
		{600,   1.0f, 0.007f, 0.0002f},
		{3250,  1.0f, 0.0014f, 0.000007f}
	};

	static Attenuation GetClosestAttenuation(float inputDistance)
	{
		const Attenuation* closest = &g_AttenuationTable[0];
		float minDiff = std::abs(inputDistance - closest->distance);

		for (const auto& a : g_AttenuationTable)
		{
			float diff = glm::abs(inputDistance - a.distance);
			if (diff < minDiff)
			{
				closest = &a;
				minDiff = diff;
			}
		}

		return *closest;
	}

	void ForwardRenderingSystem::OnCreate()
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

		DepthPrePassSetup();
		OpaquePassSetup();
		TransparentPassSetup();
		SpriteRenderingSetup();
		PostProcessPassSetup();
		PresentPassSetup();
	}

	void ForwardRenderingSystem::OnUpdate(float ts)
	{
		GetViewProjection();

		GatherDraws();
		GatherLights();

		CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::MAIN);

		ResourceManager::Instance->TransitionTextureLayout(
			commandBuffer,
			Renderer::Instance->IntermediateColorTexture,
			TextureLayout::UNDEFINED,
			TextureLayout::RENDER_ATTACHMENT,
			{}
		);

		ResourceManager::Instance->TransitionTextureLayout(
			commandBuffer,
			Renderer::Instance->MainColorTexture,
			TextureLayout::UNDEFINED,
			TextureLayout::RENDER_ATTACHMENT,
			{}
		);

		auto& renderPassPool = Renderer::Instance->GetRenderPassPool();

		renderPassPool.Execute(RenderPassEvent::BeforeRendering);

		renderPassPool.Execute(RenderPassEvent::BeforeRenderingShadows);
		ShadowPass(commandBuffer);
		renderPassPool.Execute(RenderPassEvent::AfterRenderingShadows);

		renderPassPool.Execute(RenderPassEvent::BeforeRenderingPrePasses);
		DepthPrePass(commandBuffer);
		renderPassPool.Execute(RenderPassEvent::AfterRenderingPrePasses);

		renderPassPool.Execute(RenderPassEvent::BeforeRenderingOpaques);
		OpaquePass(commandBuffer);
		renderPassPool.Execute(RenderPassEvent::AfterRenderingOpaques);

		renderPassPool.Execute(RenderPassEvent::BeforeRenderingSkybox);
		SkyboxPass(commandBuffer);
		renderPassPool.Execute(RenderPassEvent::AfterRenderingSkybox);

		renderPassPool.Execute(RenderPassEvent::BeforeRenderingTransparents);
		TransparentPass(commandBuffer);
		renderPassPool.Execute(RenderPassEvent::AfterRenderingTransparents);

		renderPassPool.Execute(RenderPassEvent::BeforeRenderingPostProcess);
		PostProcessPass(commandBuffer);
		renderPassPool.Execute(RenderPassEvent::AfterRenderingPostProcess);

		PresentPass(commandBuffer);

		renderPassPool.Execute(RenderPassEvent::AfterRendering);

		commandBuffer->EndCommandRecording();
		commandBuffer->Submit();
	}

	void ForwardRenderingSystem::OnDestroy()
	{
		m_ResourceManager->DeleteRenderPassLayout(m_RenderPassLayout);

		m_ResourceManager->DeleteShader(m_DepthOnlyShader);
		m_ResourceManager->DeleteShader(m_DepthOnlySpriteShader);

		m_ResourceManager->DeleteBindGroupLayout(m_DepthOnlyBindGroupLayout);
		m_ResourceManager->DeleteBindGroup(m_DepthOnlyBindGroup);
		m_ResourceManager->DeleteBindGroup(m_DepthOnlySpriteBindGroup);

		m_ResourceManager->DeleteMaterial(m_DepthOnlyMaterial);
		m_ResourceManager->DeleteMaterial(m_DepthOnlySpriteMaterial);

		m_ResourceManager->DeleteRenderPassLayout(m_DepthOnlyRenderPassLayout);
		m_ResourceManager->DeleteRenderPass(m_DepthOnlyRenderPass);
		m_ResourceManager->DeleteFrameBuffer(m_DepthOnlyFrameBuffer);
		Renderer::Instance->RemoveOnResizeCallback("Depth-Only-Resize-FrameBuffer");

		m_ResourceManager->DeleteRenderPass(m_OpaqueRenderPass);
		m_ResourceManager->DeleteFrameBuffer(m_OpaqueFrameBuffer);
		Renderer::Instance->RemoveOnResizeCallback("Resize-Opaque-FrameBuffer");

		m_ResourceManager->DeleteRenderPass(m_TransparentRenderPass);
		m_ResourceManager->DeleteFrameBuffer(m_TransparentFrameBuffer);
		Renderer::Instance->RemoveOnResizeCallback("Resize-Transparent-FrameBuffer");

		m_ResourceManager->DeleteBuffer(m_PostProcessBuffer);
		m_ResourceManager->DeleteShader(m_PostProcessShader);
		m_ResourceManager->DeleteBindGroupLayout(m_PostProcessBindGroupLayout);
		m_ResourceManager->DeleteBindGroup(m_PostProcessBindGroup);
		m_ResourceManager->DeleteMaterial(m_PostProcessMaterial);
		m_ResourceManager->DeleteRenderPass(m_PostProcessRenderPass);
		m_ResourceManager->DeleteFrameBuffer(m_PostProcessFrameBuffer);
		Renderer::Instance->RemoveOnResizeCallback("Post-Process-Resize-FrameBuffer");

		m_ResourceManager->DeleteBuffer(m_VertexBuffer);
		m_ResourceManager->DeleteMesh(m_SpriteMesh);

		m_ResourceManager->DeleteBuffer(m_QuadVertexBuffer);
		m_ResourceManager->DeleteMesh(m_QuadMesh);
		m_ResourceManager->DeleteMaterial(m_QuadMaterial);
	}

	void ForwardRenderingSystem::DepthPrePassSetup()
	{
		// Create pre-pass renderpass and framebuffer.
		m_DepthOnlyRenderPassLayout = m_ResourceManager->CreateRenderPassLayout({
			.debugName = "pre-pass-renderpass-layout",
			.depthTargetFormat = Format::D32_FLOAT,
			.subPasses = {
				{ .depthTarget = true },
			},
		});

		m_DepthOnlyRenderPass = m_ResourceManager->CreateRenderPass({
			.debugName = "pre-pass-renderpass",
			.layout = m_DepthOnlyRenderPassLayout,
			.depthTarget = {
				.loadOp = LoadOperation::CLEAR,
				.storeOp = StoreOperation::STORE,
				.stencilLoadOp = LoadOperation::DONT_CARE,
				.stencilStoreOp = StoreOperation::DONT_CARE,
				.prevUsage = TextureLayout::UNDEFINED,
				.nextUsage = TextureLayout::DEPTH_STENCIL,
			},
		});

		m_DepthOnlyFrameBuffer = m_ResourceManager->CreateFrameBuffer({
			.debugName = "viewport-fb",
			.width = Window::Instance->GetExtents().x,
			.height = Window::Instance->GetExtents().y,
			.renderPass = m_DepthOnlyRenderPass,
			.depthTarget = Renderer::Instance->MainDepthTexture,
		});

		Renderer::Instance->AddCallbackOnResize("Depth-Only-Resize-FrameBuffer", [this](uint32_t width, uint32_t height)
		{
			ResourceManager::Instance->DeleteFrameBuffer(m_DepthOnlyFrameBuffer);

			m_DepthOnlyFrameBuffer = ResourceManager::Instance->CreateFrameBuffer({
				.debugName = "viewport-fb",
				.width = width,
				.height = height,
				.renderPass = m_DepthOnlyRenderPass,
				.depthTarget = Renderer::Instance->MainDepthTexture,
			});
		});

		// Create pre-pass bind group.
		m_DepthOnlyBindGroupLayout = m_ResourceManager->CreateBindGroupLayout({
			.debugName = "pre-pass-bind-group-layout",
			.bufferBindings = {
				{
					.slot = 2,
					.visibility = ShaderStage::VERTEX,
					.type = BufferBindingType::UNIFORM_DYNAMIC_OFFSET,
				},
			},
		});

		m_DepthOnlyBindGroup = ResourceManager::Instance->CreateBindGroup({
			.debugName = "pre-pass-bind-group",
			.layout = m_DepthOnlyBindGroupLayout,
			.buffers = {
				{ .buffer = Renderer::Instance->TempUniformRingBuffer->GetBuffer(), .range = sizeof(PerDrawData) },
			}
		});

		m_DepthOnlySpriteBindGroup = ResourceManager::Instance->CreateBindGroup({
			.debugName = "pre-pass-bind-group",
			.layout = m_DepthOnlyBindGroupLayout,
			.buffers = {
				{ .buffer = Renderer::Instance->TempUniformRingBuffer->GetBuffer(), .range = sizeof(PerDrawDataSprite) },
			}
		});

		// Create pre-pass shaders.
		const auto& prePassShaderCode = ShaderUtilities::Get().Compile("assets/shaders/pre-pass-mesh.shader");

		ShaderDescriptor::RenderPipeline::Variant variant = {};
		variant.blend.colorOutput = false;
		variant.blend.enabled = false;
		variant.shaderHashKey = Random::UInt64(); // Create a random UUID since we do not have an asset to retrieve from there the UUID.

		m_DepthOnlyShader = ResourceManager::Instance->CreateShader({
			.debugName = "mesh-pre-pass-shader",
			.VS { .code = prePassShaderCode[0], .entryPoint = "main" },
			.FS { .code = prePassShaderCode[1], .entryPoint = "main" },
			.bindGroups {
				Renderer::Instance->GetGlobalBindingsLayout2D(),	// Global bind group (0)
				m_DepthOnlyBindGroupLayout,							// (1)
			},
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = 32,
						.attributes = {
							{ .byteOffset = 0,  .format = VertexFormat::FLOAT32x3 },
							{ .byteOffset = 12, .format = VertexFormat::FLOAT32x3 },
							{ .byteOffset = 24, .format = VertexFormat::FLOAT32x2 },
						},
					}
				},
				.variants = { variant },
			},
			.renderPass = m_DepthOnlyRenderPass,
		});

		ResourceManager::Instance->AddShaderVariant(m_DepthOnlyShader, variant);

		const auto& prePassSpriteShaderCode = ShaderUtilities::Get().Compile("assets/shaders/pre-pass-sprite.shader");

		m_DepthOnlySpriteShader = ResourceManager::Instance->CreateShader({
			.debugName = "sprite-pre-pass-shader",
			.VS { .code = prePassSpriteShaderCode[0], .entryPoint = "main" },
			.FS { .code = prePassSpriteShaderCode[1], .entryPoint = "main" },
			.bindGroups {
				Renderer::Instance->GetGlobalBindingsLayout2D(),	// Global bind group (0)
				m_DepthOnlyBindGroupLayout,							// (1)
			},
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = 20,
						.attributes = {
							{ .byteOffset = 0,  .format = VertexFormat::FLOAT32x3 },
							{ .byteOffset = 12, .format = VertexFormat::FLOAT32x2 },
						},
					}
				},
				.variants = { variant },
			},
			.renderPass = m_DepthOnlyRenderPass,
		});

		ResourceManager::Instance->AddShaderVariant(m_DepthOnlySpriteShader, variant);

		// Create pre-pass materials.
		m_DepthOnlyMaterial = ResourceManager::Instance->CreateMaterial({
			.debugName = "depth-only-mesh-material",
			.shader = m_DepthOnlyShader,
			.bindGroup = m_DepthOnlyBindGroup,
		});

		Material* mat0 = ResourceManager::Instance->GetMaterial(m_DepthOnlyMaterial);
		mat0->VariantDescriptor = variant;

		m_DepthOnlySpriteMaterial = ResourceManager::Instance->CreateMaterial({
			.debugName = "depth-only-sprite-material",
			.shader = m_DepthOnlySpriteShader,
			.bindGroup = m_DepthOnlySpriteBindGroup,
		});

		Material* mat1 = ResourceManager::Instance->GetMaterial(m_DepthOnlySpriteMaterial);
		mat1->VariantDescriptor = variant;
	}

	void ForwardRenderingSystem::OpaquePassSetup()
	{
		// Renderpass and framebuffer for opaques.
		m_OpaqueRenderPass = m_ResourceManager->CreateRenderPass({
			.debugName = "opaques-renderpass",
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
					.format = Format::RGBA16_FLOAT,
					.loadOp = LoadOperation::CLEAR,
					.storeOp = StoreOperation::STORE,
					.prevUsage = TextureLayout::UNDEFINED,
					.nextUsage = TextureLayout::RENDER_ATTACHMENT,
				},
			},
		});

		m_OpaqueFrameBuffer = m_ResourceManager->CreateFrameBuffer({
			.debugName = "opaques-viewport-fb",
			.width = Window::Instance->GetExtents().x,
			.height = Window::Instance->GetExtents().y,
			.renderPass = m_OpaqueRenderPass,
			.depthTarget = Renderer::Instance->MainDepthTexture,
			.colorTargets = { Renderer::Instance->IntermediateColorTexture },
		});
		
		// Resize opaque framebuffer callback.
		Renderer::Instance->AddCallbackOnResize("Resize-Opaque-FrameBuffer", [this](uint32_t width, uint32_t height)
		{
			ResourceManager::Instance->DeleteFrameBuffer(m_OpaqueFrameBuffer);

			m_OpaqueFrameBuffer = ResourceManager::Instance->CreateFrameBuffer({
				.debugName = "opaques-viewport-fb",
				.width = width,
				.height = height,
				.renderPass = m_OpaqueRenderPass,
				.depthTarget = Renderer::Instance->MainDepthTexture,
				.colorTargets = { Renderer::Instance->IntermediateColorTexture },
			});
		});
	}

	void ForwardRenderingSystem::TransparentPassSetup()
	{
		// Renderpass and framebuffer for transparents.
		m_TransparentRenderPass = m_ResourceManager->CreateRenderPass({
			.debugName = "transparents-renderpass",
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
					.format = Format::RGBA16_FLOAT,
					.loadOp = LoadOperation::LOAD,
					.storeOp = StoreOperation::STORE,
					.prevUsage = TextureLayout::RENDER_ATTACHMENT,
					.nextUsage = TextureLayout::RENDER_ATTACHMENT,
				},
			},
		});

		m_TransparentFrameBuffer = m_ResourceManager->CreateFrameBuffer({
			.debugName = "transparents-viewport-fb",
			.width = Window::Instance->GetExtents().x,
			.height = Window::Instance->GetExtents().y,
			.renderPass = m_TransparentRenderPass,
			.depthTarget = Renderer::Instance->MainDepthTexture,
			.colorTargets = { Renderer::Instance->IntermediateColorTexture },
		});

		// Resize transparent framebuffer callback.
		Renderer::Instance->AddCallbackOnResize("Resize-Transparent-FrameBuffer", [this](uint32_t width, uint32_t height)
		{
			ResourceManager::Instance->DeleteFrameBuffer(m_TransparentFrameBuffer);

			m_TransparentFrameBuffer = ResourceManager::Instance->CreateFrameBuffer({
				.debugName = "transparents-viewport-fb",
				.width = width,
				.height = height,
				.renderPass = m_TransparentRenderPass,
				.depthTarget = Renderer::Instance->MainDepthTexture,
				.colorTargets = { Renderer::Instance->IntermediateColorTexture },
			});
		});
	}

	void ForwardRenderingSystem::SpriteRenderingSetup()
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
	}

	void ForwardRenderingSystem::SkyboxPassSetup()
	{
	}

	void ForwardRenderingSystem::PostProcessPassSetup()
	{
		// Create camera settings buffer.
		m_PostProcessBuffer = m_ResourceManager->CreateBuffer({
			.debugName = "camera-settings-buffer",
			.usage = BufferUsage::UNIFORM,
			.usageHint = BufferUsageHint::DYNAMIC,
			.byteSize = sizeof(CameraSettings),
		});

		// Create post-process bind group.
		m_PostProcessBindGroupLayout = m_ResourceManager->CreateBindGroupLayout({
			.debugName = "post-process-bind-group-layout",
			.textureBindings = {
				{
					.slot = 0,
					.visibility = ShaderStage::FRAGMENT,
				},
			},
			.bufferBindings = {
				{
					.slot = 1,
					.visibility = ShaderStage::FRAGMENT,
					.type = BufferBindingType::UNIFORM,
				},
			},
		});

		m_PostProcessBindGroup = ResourceManager::Instance->CreateBindGroup({
			.debugName = "post-process-bind-group",
			.layout = m_PostProcessBindGroupLayout,
			.textures = {
				Renderer::Instance->IntermediateColorTexture
			},
			.buffers = {
				{.buffer = m_PostProcessBuffer },
			}
		});

		// Create post-process renderpass and framebuffer.
		m_PostProcessRenderPass = m_ResourceManager->CreateRenderPass({
			.debugName = "post-process-renderpass",
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
					.loadOp = LoadOperation::CLEAR,
					.storeOp = StoreOperation::STORE,
					.prevUsage = TextureLayout::UNDEFINED,
					.nextUsage = TextureLayout::RENDER_ATTACHMENT,
				},
			},
		});

		m_PostProcessFrameBuffer = m_ResourceManager->CreateFrameBuffer({
			.debugName = "post-process-fb",
			.width = Window::Instance->GetExtents().x,
			.height = Window::Instance->GetExtents().y,
			.renderPass = m_PostProcessRenderPass,
			.depthTarget = Renderer::Instance->MainDepthTexture,
			.colorTargets = { Renderer::Instance->MainColorTexture },
		});

		Renderer::Instance->AddCallbackOnResize("Post-Process-Resize-FrameBuffer", [this](uint32_t width, uint32_t height)
		{
			ResourceManager::Instance->DeleteFrameBuffer(m_PostProcessFrameBuffer);

			m_PostProcessFrameBuffer = m_ResourceManager->CreateFrameBuffer({
				.debugName = "post-process-fb",
				.width = width,
				.height = height,
				.renderPass = m_PostProcessRenderPass,
				.depthTarget = Renderer::Instance->MainDepthTexture,
				.colorTargets = { Renderer::Instance->MainColorTexture },
			});

			ResourceManager::Instance->DeleteBindGroup(m_PostProcessBindGroup);

			m_PostProcessBuffer = m_ResourceManager->CreateBuffer({
				.debugName = "camera-settings-buffer",
				.usage = BufferUsage::UNIFORM,
				.usageHint = BufferUsageHint::DYNAMIC,
				.byteSize = sizeof(CameraSettings),
			});

			m_PostProcessBindGroup = ResourceManager::Instance->CreateBindGroup({
				.debugName = "post-process-bind-group",
				.layout = m_PostProcessBindGroupLayout,
				.textures = {
					Renderer::Instance->IntermediateColorTexture
				},
				.buffers = {
					{ .buffer = m_PostProcessBuffer },
				}
			});
		});

		// Create pre-pass shaders.
		const auto& postProcessShaderCode = ShaderUtilities::Get().Compile("assets/shaders/post-process-tone-mapping.shader");

		ShaderDescriptor::RenderPipeline::Variant variant = {};
		variant.blend.enabled = false;
		variant.shaderHashKey = Random::UInt64(); // Create a random UUID since we do not have an asset to retrieve from there the UUID.

		m_PostProcessShader = ResourceManager::Instance->CreateShader({
			.debugName = "post-process-shader",
			.VS {.code = postProcessShaderCode[0], .entryPoint = "main" },
			.FS {.code = postProcessShaderCode[1], .entryPoint = "main" },
			.bindGroups {
				m_PostProcessBindGroupLayout,	// Global bind group (0)
			},
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = 16,
						.attributes = {
							{.byteOffset = 0, .format = VertexFormat::FLOAT32x2 },
							{.byteOffset = 8, .format = VertexFormat::FLOAT32x2 },
						},
					}
				},
				.variants = { variant },
			},
			.renderPass = m_PostProcessRenderPass,
		});

		ResourceManager::Instance->AddShaderVariant(m_PostProcessShader, variant);

		// Create post-process material.
		m_PostProcessMaterial = ResourceManager::Instance->CreateMaterial({
			.debugName = "post-process-material",
			.shader = m_PostProcessShader,
			.bindGroup = m_PostProcessBindGroup,
		});

		Material* mat = ResourceManager::Instance->GetMaterial(m_PostProcessMaterial);
		mat->VariantDescriptor = variant;
	}

	void ForwardRenderingSystem::PresentPassSetup()
	{
		float* vertexBuffer = nullptr;

		// if (Renderer::Instance->GetAPI() == GraphicsAPI::VULKAN)
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
		//else if (Renderer::Instance->GetAPI() == GraphicsAPI::OPENGL)
		//{
		//	vertexBuffer = new float[24] {
		//		-1.0, -1.0, 0.0, 0.0, // 0 - Bottom left
		//		 1.0, -1.0, 1.0, 0.0, // 1 - Bottom right
		//		 1.0,  1.0, 1.0, 1.0, // 2 - Top right
		//		 1.0,  1.0, 1.0, 1.0, // 2 - Top right
		//		-1.0,  1.0, 0.0, 1.0, // 3 - Top left
		//		-1.0, -1.0, 0.0, 0.0, // 0 - Bottom left
		//	};
		//}

		m_QuadVertexBuffer = m_ResourceManager->CreateBuffer({
			.debugName = "quad-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.byteSize = sizeof(float) * 24,
			.initialData = vertexBuffer,
		});

		m_QuadMesh = m_ResourceManager->CreateMesh({
			.debugName = "quad-mesh",
			.meshes = {
				{
					.debugName = "quad-mesh-part",
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

	bool ForwardRenderingSystem::IsInFrustum(const Component::Transform& transform)
	{
		glm::vec3 worldPosition = glm::normalize(glm::vec3(transform.LocalMatrix * glm::vec4(transform.Translation, 1.0f)));
		float radius = glm::length(transform.Scale) * 0.5f;

		for (const auto& plane : m_CameraFrustum.Planes)
		{
			float distance = glm::dot(plane.normal, worldPosition) + plane.distance;

			// If the sphere is completely outside the frustum, reject it
			if (distance < -radius)
			{
				return false; // Outside
			}
		}

		return true; // Inside or intersecting
	}

	bool ForwardRenderingSystem::IsInFrustum(Handle<Mesh> meshHandle, const Component::Transform& transform)
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

	void ForwardRenderingSystem::GatherDraws()
	{
		// Static meshes
		{
			// Store the offset that the static meshes start from in the dynamic uniform buffer.
			m_UBOStaticMeshOffset = m_UniformRingBuffer->GetCurrentOffset();

			m_StaticMeshOpaqueDraws.Reset();
			m_StaticMeshTransparentDraws.Reset();
			m_PrePassStaticMeshDraws.Reset();

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
						alloc.Data->InverseModel = glm::transpose(glm::inverse(glm::mat3(transform.WorldMatrix)));
						alloc.Data->Color = material->AlbedoColor;
						alloc.Data->Glossiness = material->Glossiness;

						if (!material->VariantDescriptor.blend.enabled)
						{
							m_StaticMeshOpaqueDraws.Insert({
								.Shader = material->Shader,
								.BindGroup = material->BindGroup,
								.Mesh = staticMesh.Mesh,
								.MeshIndex = staticMesh.MeshIndex,
								.SubMeshIndex = staticMesh.SubMeshIndex,
								.Material = staticMesh.Material,
								.Offset = alloc.Offset,
								.Size = sizeof(PerDrawData),
							});

							// Include only opaque objects in depth pre-pass.
							m_PrePassStaticMeshDraws.Insert({
								.Shader = m_DepthOnlyShader,
								.BindGroup = m_DepthOnlyBindGroup,
								.Mesh = staticMesh.Mesh,
								.MeshIndex = staticMesh.MeshIndex,
								.SubMeshIndex = staticMesh.SubMeshIndex,
								.Material = m_DepthOnlyMaterial,
								.Offset = alloc.Offset,
								.Size = sizeof(PerDrawData),
							});
						}
						else
						{
							m_StaticMeshTransparentDraws.Insert({
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
					}
				});

			// Store the total size of the static mesh draw data in the dynamic uniform data.
			uint32_t totalDraws = m_StaticMeshOpaqueDraws.GetCount() + m_StaticMeshTransparentDraws.GetCount();
			m_UBOStaticMeshSize = totalDraws * m_UniformRingBuffer->GetAlignedSize(sizeof(PerDrawData));
		}

		// Sprites
		{
			// Store the offset that the sprites start from in the dynamic uniform buffer.
			m_UBOSpriteOffset = m_UniformRingBuffer->GetCurrentOffset();

			m_SpriteOpaqueDraws.Reset();
			m_SpriteTransparentDraws.Reset();
			m_PrePassSpriteDraws.Reset();

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

							if (!material->VariantDescriptor.blend.enabled)
							{
								m_SpriteOpaqueDraws.Insert({
									.Shader = material->Shader,
									.BindGroup = material->BindGroup,
									.Mesh = m_SpriteMesh,
									.Material = sprite.Material,
									.Offset = alloc.Offset,
									.Size = sizeof(PerDrawDataSprite),
								});

								// Include only opaque objects in depth pre-pass.
								m_PrePassSpriteDraws.Insert({
									.Shader = m_DepthOnlySpriteShader,
									.BindGroup = m_DepthOnlySpriteBindGroup,
									.Mesh = m_SpriteMesh,
									.Material = m_DepthOnlySpriteMaterial,
									.Offset = alloc.Offset,
									.Size = sizeof(PerDrawDataSprite),
								});
							}
							else
							{
								m_SpriteTransparentDraws.Insert({
									.Shader = material->Shader,
									.BindGroup = material->BindGroup,
									.Mesh = m_SpriteMesh,
									.Material = sprite.Material,
									.Offset = alloc.Offset,
									.Size = sizeof(PerDrawDataSprite),
								});
							}
						}
					});

			// Store the total size of the sprite draw data in the dynamic uniform data.
			uint32_t totalDraws = m_SpriteOpaqueDraws.GetCount() + m_SpriteTransparentDraws.GetCount();
			m_UBOSpriteSize = totalDraws * m_UniformRingBuffer->GetAlignedSize(sizeof(PerDrawDataSprite));
		}
	}

	void ForwardRenderingSystem::GatherLights()
	{
		m_LightData.LightCount = 0;
		m_Context->GetRegistry()
			.group<Component::Light>(entt::get<Component::Transform>)
			.each([&](Component::Light& light, Component::Transform& transform)
			{
				if (light.Enabled)
				{
					float lightType = 0.0f;

					const Attenuation& attenuation = GetClosestAttenuation(light.Distance);

					switch (light.Type)
					{
					case Component::Light::Type::Directional:
						lightType = 0.0f;
						break;
					case Component::Light::Type::Point:
						lightType = 1.0f;
						m_LightData.LightMetadata[(int)m_LightData.LightCount].y = attenuation.constant;
						m_LightData.LightMetadata[(int)m_LightData.LightCount].z = attenuation.linear;
						m_LightData.LightMetadata[(int)m_LightData.LightCount].w = attenuation.quadratic;
						break;
					case Component::Light::Type::Spot:
						lightType = 2.0f;
						m_LightData.LightMetadata[(int)m_LightData.LightCount].y = glm::cos(glm::radians(light.InnerCutOff));
						m_LightData.LightMetadata[(int)m_LightData.LightCount].z = glm::cos(glm::radians(light.OuterCutOff));
						break;
					}

					// Convert rotation from degrees to radians
					glm::vec3 rotationRadians = glm::radians(transform.Rotation);

					// Create local rotation matrix from Euler angles (YXZ order is common, but adjust as needed)
					glm::mat4 localRotation = glm::eulerAngleYXZ(rotationRadians.y, rotationRadians.x, rotationRadians.z);

					// Define the local forward direction
					glm::vec3 localForward = glm::vec3(0.0f, -1.0f, 0.0f);

					// Transform local forward by the local rotation matrix
					glm::vec4 rotatedDirection = localRotation * glm::vec4(localForward, 0.0f); // w = 0 to avoid translation

					// Extract rotation part of world matrix (3x3)
					glm::mat3 worldRotation = glm::mat3(transform.WorldMatrix);

					// Apply world rotation
					glm::vec3 worldDirection = glm::normalize(worldRotation * glm::vec3(rotatedDirection));

					m_LightData.LightPositions[(int)m_LightData.LightCount] = transform.WorldMatrix * glm::vec4(transform.Translation, 1.0f);
					m_LightData.LightPositions[(int)m_LightData.LightCount].w = lightType;
					m_LightData.LightDirections[(int)m_LightData.LightCount] = glm::vec4(worldDirection, 0.0f);
					m_LightData.LightMetadata[(int)m_LightData.LightCount].x = light.Intensity;
					m_LightData.LightColors[(int)m_LightData.LightCount] = glm::vec4(light.Color, 1.0f);
					m_LightData.LightCount++;
				}
			});
	}

	void ForwardRenderingSystem::ShadowPass(CommandBuffer* commandBuffer)
	{
	}

	void ForwardRenderingSystem::DepthPrePass(CommandBuffer* commandBuffer)
	{
		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_DepthOnlyRenderPass, m_DepthOnlyFrameBuffer);

		Handle<BindGroup> globalBindings = Renderer::Instance->GetGlobalBindings2D();

		// Depth only pre pass for opaque static meshes.
		{
			ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&m_CameraData.ViewProjection);
			GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings, .DynamicUniformBufferOffset = m_UBOStaticMeshOffset, .DynamicUniformBufferSize = m_UBOStaticMeshSize };
			passRenderer->DrawSubPass(globalDrawStream, m_PrePassStaticMeshDraws);
		}

		// Depth only pre pass for opaque sprites.
		{
			ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&m_CameraData.ViewProjection);
			GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings, .DynamicUniformBufferOffset = m_UBOSpriteOffset, .DynamicUniformBufferSize = m_UBOSpriteSize };
			passRenderer->DrawSubPass(globalDrawStream, m_PrePassSpriteDraws);
		}

		commandBuffer->EndRenderPass(*passRenderer);
	}

	void ForwardRenderingSystem::OpaquePass(CommandBuffer* commandBuffer)
	{
		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_OpaqueRenderPass, m_OpaqueFrameBuffer);

		// Render opaque meshes.
		{
			Handle<BindGroup> globalBindings = Renderer::Instance->GetGlobalBindings3D();
			ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&m_CameraData);
			ResourceManager::Instance->SetBufferData(globalBindings, 1, (void*)&m_LightData);
			GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings, .DynamicUniformBufferOffset = m_UBOStaticMeshOffset, .DynamicUniformBufferSize = m_UBOStaticMeshSize };
			passRenderer->DrawSubPass(globalDrawStream, m_StaticMeshOpaqueDraws);
		}

		// Render opaque sprites.
		{
			Handle<BindGroup> globalBindings = Renderer::Instance->GetGlobalBindings2D();
			ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&m_CameraData.ViewProjection);
			GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings, .DynamicUniformBufferOffset = m_UBOSpriteOffset, .DynamicUniformBufferSize = m_UBOSpriteSize };
			passRenderer->DrawSubPass(globalDrawStream, m_SpriteOpaqueDraws);
		}

		commandBuffer->EndRenderPass(*passRenderer);
	}

	void ForwardRenderingSystem::TransparentPass(CommandBuffer* commandBuffer)
	{
		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_TransparentRenderPass, m_TransparentFrameBuffer);

		// Render transparent meshes.
		{
			Handle<BindGroup> globalBindings = Renderer::Instance->GetGlobalBindings3D();
			ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&m_CameraData);
			ResourceManager::Instance->SetBufferData(globalBindings, 1, (void*)&m_LightData);
			GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings, .DynamicUniformBufferOffset = m_UBOStaticMeshOffset, .DynamicUniformBufferSize = m_UBOStaticMeshSize };
			passRenderer->DrawSubPass(globalDrawStream, m_StaticMeshTransparentDraws);
		}

		// Render transparent sprites.
		{
			Handle<BindGroup> globalBindings = Renderer::Instance->GetGlobalBindings2D();
			ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&m_CameraData.ViewProjection);
			GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings, .DynamicUniformBufferOffset = m_UBOSpriteOffset, .DynamicUniformBufferSize = m_UBOSpriteSize };
			passRenderer->DrawSubPass(globalDrawStream, m_SpriteTransparentDraws);
		}

		commandBuffer->EndRenderPass(*passRenderer);
	}

	void ForwardRenderingSystem::SkyboxPass(CommandBuffer* commandBuffer)
	{
		/*m_Context->GetRegistry()
			.group<Component::SkyLight>(entt::get<Component::Transform>)
			.each([&](Component::SkyLight& skyLight, Component::Transform& transform)
			{
				if (skyLight.Enabled)
				{
					if (!skyLight.Converted)
					{
						glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
						glm::mat4 captureViews[] =
						{
							glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
							glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
							glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
							glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
							glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
							glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
						};

						CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::CUSTOM, RenderPassStage::Skybox);
						RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_PostProcessRenderPass, m_PostProcessFrameBuffer);

						DrawList draws;

						for (uint32_t i = 0; i < 6; ++i)
						{
							draws.Insert({
								.Shader = m_PostProcessShader,
								.Mesh = m_QuadMesh,
								.Material = m_PostProcessMaterial,
							});
						}

						ResourceManager::Instance->SetBufferData(m_PostProcessBindGroup, 0, (void*)&m_CameraSettings);
						GlobalDrawStream globalDrawStream = { .BindGroup = m_PostProcessBindGroup };
						passRenderer->DrawSubPass(globalDrawStream, draws);

						commandBuffer->EndRenderPass(*passRenderer);
						commandBuffer->Submit();
					}
				}
			});*/
	}

	void ForwardRenderingSystem::PostProcessPass(CommandBuffer* commandBuffer)
	{
		// Transition the layout of the texture that the scene is rendered to, in order to be sampled in the shader.
		ResourceManager::Instance->TransitionTextureLayout(
			commandBuffer,
			Renderer::Instance->IntermediateColorTexture,
			TextureLayout::RENDER_ATTACHMENT,
			TextureLayout::SHADER_READ_ONLY,
			m_PostProcessBindGroup);

		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_PostProcessRenderPass, m_PostProcessFrameBuffer);

		DrawList draws;
		draws.Insert({
			.Shader = m_PostProcessShader,
			.Mesh = m_QuadMesh,
			.Material = m_PostProcessMaterial,
		});

		ResourceManager::Instance->SetBufferData(m_PostProcessBindGroup, 0, (void*)&m_CameraSettings);
		GlobalDrawStream globalDrawStream = { .BindGroup = m_PostProcessBindGroup };
		passRenderer->DrawSubPass(globalDrawStream, draws);

		commandBuffer->EndRenderPass(*passRenderer);
	}

	void ForwardRenderingSystem::PresentPass(CommandBuffer* commandBuffer)
	{
		// Transition the layout of the texture that the scene is rendered to, in order to be sampled in the shader.
		ResourceManager::Instance->TransitionTextureLayout(
			commandBuffer,
			Renderer::Instance->MainColorTexture,
			TextureLayout::RENDER_ATTACHMENT,
			TextureLayout::SHADER_READ_ONLY,
			Renderer::Instance->GetGlobalPresentBindings());

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
	}

	void ForwardRenderingSystem::GetViewProjection()
	{
		if (Context::Mode == Mode::Runtime)
		{
			if (m_Context->MainCamera != entt::null)
			{
				Component::Camera& camera = m_Context->GetComponent<Component::Camera>(m_Context->MainCamera);
				m_CameraSettings.Exposure = camera.Exposure;
				m_CameraSettings.Gamma = camera.Gamma;
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
				m_CameraSettings.Exposure = camera.Exposure;
				m_CameraSettings.Gamma = camera.Gamma;
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
		m_CameraSettings.Exposure = 1.0f;
		m_CameraSettings.Gamma = 2.2f;
		m_CameraFrustum = {};
	}
}
