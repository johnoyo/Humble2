#include "ForwardSceneRenderer.h"

#include "Core/Window.h"
#include "Core/Context.h"
#include "Renderer/DebugRenderer.h"
#include "Renderer/Device.h"
#include "Utilities/ShaderUtilities.h"

#include <glm/gtx/euler_angles.hpp>

namespace HBL2
{
#ifdef DIST
	#define BEGIN_PROFILE_PASS()
	#define END_PROFILE_PASS(time)
#else
	#define BEGIN_PROFILE_PASS() Timer profilePass
	#define END_PROFILE_PASS(time) time = profilePass.ElapsedMillis()
#endif

	struct Attenuation
	{
		float distance;
		float constant;
		float linear;
		float quadratic;
	};

	static const std::vector<Attenuation> g_AttenuationTable =
	{
		{7,    1.0f, 0.7f,    1.8f},
		{13,   1.0f, 0.35f,   0.44f},
		{20,   1.0f, 0.22f,   0.20f},
		{32,   1.0f, 0.14f,   0.07f},
		{50,   1.0f, 0.09f,   0.032f},
		{65,   1.0f, 0.07f,   0.017f},
		{100,  1.0f, 0.045f,  0.0075f},
		{160,  1.0f, 0.027f,  0.0028f},
		{200,  1.0f, 0.022f,  0.0019f},
		{325,  1.0f, 0.014f,  0.0007f},
		{600,  1.0f, 0.007f,  0.0002f},
		{3250, 1.0f, 0.0014f, 0.000007f}
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

	struct CaptureMatrices
	{
		glm::mat4 View[6];
		float FaceSize = 2048.f;
		float _padding[3];
	};

	static CaptureMatrices g_CaptureMatrices =
	{
		.View =
		{
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		}
	};

	void ForwardSceneRenderer::Initialize(Scene* scene)
	{
		m_Scene = scene;

		m_ResourceManager = ResourceManager::Instance;
		m_EditorScene = m_ResourceManager->GetScene(Context::EditorScene);
		m_UniformRingBuffer = Renderer::Instance->TempUniformRingBuffer;

		Renderer::Instance->SubmitBlocking([this]()
		{
			// Create color render pass.
			m_RenderPassLayout = m_ResourceManager->CreateRenderPassLayout({
				.debugName = "main-renderpass-layout",
				.depthTargetFormat = Format::D32_FLOAT,
				.subPasses = {
					{ .depthTarget = true, .colorTargets = 1, },
				},
			});

			// Create depth only render pass.
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

			// Create pre-pass bind groups.
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

			m_DepthOnlyMeshBindGroup = ResourceManager::Instance->CreateBindGroup({
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

			// Setup render passes.
			ShadowPassSetup();
			DepthPrePassSetup();
			OpaquePassSetup();
			TransparentPassSetup();
			SpriteRenderingSetup();
			PostProcessPassSetup();
			SkyboxPassSetup();
			PresentPassSetup();
		});
	}

	void ForwardSceneRenderer::Gather(Entity mainCamera)
	{
		SceneRenderData* sceneRenderData = &m_RenderData[Renderer::Instance->GetFrameWriteIndex()];

		GetViewProjection(sceneRenderData, mainCamera);

		GatherDraws(sceneRenderData);
		GatherLights(sceneRenderData);
	}

	void ForwardSceneRenderer::Render(void* renderData)
	{
		SceneRenderData* sceneRenderData = (SceneRenderData*)renderData;
		UniformRingBuffer* uniformRingBuffer = Renderer::Instance->TempUniformRingBuffer;

		// Map dynamic uniform buffer data (i.e.: Bump allocated per object data)
		ResourceManager::Instance->MapBufferData(uniformRingBuffer->GetBuffer(), sceneRenderData->m_UBOStartingOffset, sceneRenderData->m_UBOEndingOffset - sceneRenderData->m_UBOStartingOffset);

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

		ResourceManager::Instance->TransitionTextureLayout(
			commandBuffer,
			Renderer::Instance->ShadowAtlasTexture,
			TextureLayout::UNDEFINED,
			TextureLayout::DEPTH_STENCIL,
			{}
		);

		auto& renderPassPool = Renderer::Instance->GetRenderPassPool();

		renderPassPool.Execute(RenderPassEvent::BeforeRendering);

		renderPassPool.Execute(RenderPassEvent::BeforeRenderingShadows);
		ShadowPass(commandBuffer, sceneRenderData);
		renderPassPool.Execute(RenderPassEvent::AfterRenderingShadows);

		renderPassPool.Execute(RenderPassEvent::BeforeRenderingPrePasses);
		DepthPrePass(commandBuffer, sceneRenderData);
		renderPassPool.Execute(RenderPassEvent::AfterRenderingPrePasses);

		renderPassPool.Execute(RenderPassEvent::BeforeRenderingOpaques);
		OpaquePass(commandBuffer, sceneRenderData);
		renderPassPool.Execute(RenderPassEvent::AfterRenderingOpaques);

		renderPassPool.Execute(RenderPassEvent::BeforeRenderingSkybox);
		SkyboxPass(commandBuffer, sceneRenderData);
		renderPassPool.Execute(RenderPassEvent::AfterRenderingSkybox);

		renderPassPool.Execute(RenderPassEvent::BeforeRenderingTransparents);
		TransparentPass(commandBuffer, sceneRenderData);
		renderPassPool.Execute(RenderPassEvent::AfterRenderingTransparents);

		renderPassPool.Execute(RenderPassEvent::BeforeRenderingPostProcess);
		PostProcessPass(commandBuffer, sceneRenderData);
		renderPassPool.Execute(RenderPassEvent::AfterRenderingPostProcess);

		DebugPass(commandBuffer, sceneRenderData);

		renderPassPool.Execute(RenderPassEvent::BeforePresenting);
		PresentPass(commandBuffer, sceneRenderData);
		renderPassPool.Execute(RenderPassEvent::AfterRendering);

		commandBuffer->EndCommandRecording();
		commandBuffer->Submit();
	}

	void ForwardSceneRenderer::CleanUp()
	{
		// TODO: Fix!
		Renderer::Instance->SubmitBlocking([this]()
		{
			m_ResourceManager->DeleteRenderPassLayout(m_RenderPassLayout);

			m_ResourceManager->DeleteTexture(m_ShadowDepthTexture);
			m_ResourceManager->DeleteFrameBuffer(m_ShadowFrameBuffer);
			m_ResourceManager->DeleteRenderPass(m_ShadowRenderPass);
			m_ResourceManager->DeleteShader(m_ShadowPrePassShader);
			m_ResourceManager->DeleteMaterial(m_ShadowPrePassMaterial);

			m_ResourceManager->DeleteShader(m_DepthOnlyShader);
			m_ResourceManager->DeleteShader(m_DepthOnlySpriteShader);

			m_ResourceManager->DeleteBindGroupLayout(m_DepthOnlyBindGroupLayout);
			m_ResourceManager->DeleteBindGroup(m_DepthOnlyMeshBindGroup);
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

			m_ResourceManager->DeleteBindGroupLayout(m_EquirectToSkyboxBindGroupLayout);
			m_ResourceManager->DeleteShader(m_EquirectToSkyboxShader);
			m_ResourceManager->DeleteBuffer(m_CaptureMatricesBuffer);
			m_ResourceManager->DeleteBindGroupLayout(m_SkyboxGlobalBindGroupLayout);
			m_ResourceManager->DeleteBindGroup(m_SkyboxGlobalBindGroup);
			m_ResourceManager->DeleteShader(m_SkyboxShader);
			m_ResourceManager->DeleteBindGroupLayout(m_SkyboxBindGroupLayout);
			m_ResourceManager->DeleteBuffer(m_CubeMeshBuffer);
			m_ResourceManager->DeleteMesh(m_CubeMesh);

			m_Scene->GetRegistry()
				.view<Component::SkyLight>()
				.each([&](Component::SkyLight& skyLight)
				{
					m_ResourceManager->DeleteTexture(skyLight.CubeMap);
					skyLight.CubeMap = {};

					Material* mat = m_ResourceManager->GetMaterial(skyLight.CubeMapMaterial);
					if (mat != nullptr)
					{
						m_ResourceManager->DeleteBindGroup(mat->BindGroup);
					}

					m_ResourceManager->DeleteMaterial(skyLight.CubeMapMaterial);
					skyLight.CubeMapMaterial = {};

					skyLight.Converted = false;
				});

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

			m_ResourceManager->DeleteBuffer(m_PostProcessQuadVertexBuffer);
			m_ResourceManager->DeleteBuffer(m_QuadVertexBuffer);
			m_ResourceManager->DeleteShader(m_PresentShader);
			m_ResourceManager->DeleteMaterial(m_QuadMaterial);
		});
	}

	void* ForwardSceneRenderer::GetRenderData()
	{
		return &m_RenderData[Renderer::Instance->GetFrameWriteIndex()];
	}

	// Pass setup.
	void ForwardSceneRenderer::ShadowPassSetup()
	{
		// Create shadow framebuffer.
		m_ShadowRenderPass = m_ResourceManager->CreateRenderPass({
			.debugName = "shadow-load-renderpass",
			.layout = m_DepthOnlyRenderPassLayout,
			.depthTarget = {
				.loadOp = LoadOperation::CLEAR,
				.storeOp = StoreOperation::STORE,
				.stencilLoadOp = LoadOperation::DONT_CARE,
				.stencilStoreOp = StoreOperation::DONT_CARE,
				.prevUsage = TextureLayout::DEPTH_STENCIL,
				.nextUsage = TextureLayout::DEPTH_STENCIL,
			},
		});

		m_ShadowFrameBuffer = m_ResourceManager->CreateFrameBuffer({
			.debugName = "shadow-fb-load",
			.width = g_ShadowAtlasSize,
			.height = g_ShadowAtlasSize,
			.renderPass = m_ShadowRenderPass,
			.depthTarget = Renderer::Instance->ShadowAtlasTexture,
		});

		// Create shadow pre-pass shader.
		const auto& shadowPrePassShaderCode = ShaderUtilities::Get().Compile("assets/shaders/shadow-mapping-pre-pass.shader");

		ShaderDescriptor::RenderPipeline::Variant variant = {};
		variant.blend.colorOutput = false;
		variant.blend.enabled = false;
		variant.cullMode = CullMode::FRONT;
		variant.shaderHashKey = Random::UInt64(); // Create a random UUID since we do not have an asset to retrieve from there the UUID.

		m_ShadowPrePassShader = ResourceManager::Instance->CreateShader({
			.debugName = "shadow-pre-pass-shader",
			.VS { .code = shadowPrePassShaderCode[0], .entryPoint = "main" },
			.FS { .code = shadowPrePassShaderCode[1], .entryPoint = "main" },
			.bindGroups {
				Renderer::Instance->GetShadowBindingsLayout(),	// Global bind group (0)
				m_DepthOnlyBindGroupLayout,						// (1)
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
			.renderPass = m_ShadowRenderPass,
		});

		// Create shadow pre-pass material.
		m_ShadowPrePassMaterial = ResourceManager::Instance->CreateMaterial({
			.debugName = "shadow-pre-pass-material",
			.shader = m_ShadowPrePassShader,
			.bindGroup = m_DepthOnlyMeshBindGroup,
		});

		Material* mat = ResourceManager::Instance->GetMaterial(m_ShadowPrePassMaterial);
		mat->VariantDescriptor = variant;

		m_ShadowPrePassMaterialHash = ResourceManager::Instance->GetShaderVariantHash(variant);
	}

	void ForwardSceneRenderer::DepthPrePassSetup()
	{
		// Create pre-pass framebuffer.	
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

		// Create pre-pass shaders.
		const auto& prePassShaderCode = ShaderUtilities::Get().Compile("assets/shaders/depth-pre-pass-mesh.shader");

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

		const auto& prePassSpriteShaderCode = ShaderUtilities::Get().Compile("assets/shaders/depth-pre-pass-sprite.shader");

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

		// Create pre-pass materials.
		m_DepthOnlyMaterial = ResourceManager::Instance->CreateMaterial({
			.debugName = "depth-only-mesh-material",
			.shader = m_DepthOnlyShader,
			.bindGroup = m_DepthOnlyMeshBindGroup,
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

		m_DepthOnlyMaterialHash = ResourceManager::Instance->GetShaderVariantHash(variant);
		m_DepthOnlySpriteMaterialHash = m_DepthOnlyMaterialHash;
	}

	void ForwardSceneRenderer::OpaquePassSetup()
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

	void ForwardSceneRenderer::TransparentPassSetup()
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

	void ForwardSceneRenderer::SpriteRenderingSetup()
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

	void ForwardSceneRenderer::SkyboxPassSetup()
	{
		m_CaptureMatricesBuffer = m_ResourceManager->CreateBuffer({
			.debugName = "capture-matrices-buffer",
			.byteSize = sizeof(CaptureMatrices),
			.initialData = &g_CaptureMatrices,
		});

		// Compile compute shaders.
		const auto& computeShaderCode = ShaderUtilities::Get().Compile("assets/shaders/equirectangular-to-skybox.shader");

		// Reflect shader.
		const auto& reflectionData = ShaderUtilities::Get().GetReflectionData("assets/shaders/equirectangular-to-skybox.shader");

		// Create compute bind group layout.
		m_EquirectToSkyboxBindGroupLayout = m_ResourceManager->CreateBindGroupLayout({
			.debugName = "compute-bind-group-layout",
			.textureBindings = {
				{
					.slot = 0,
					.visibility = ShaderStage::COMPUTE,
					.type = TextureBindingType::IMAGE_SAMPLER,
				},
				{
					.slot = 1,
					.visibility = ShaderStage::COMPUTE,
					.type = TextureBindingType::STORAGE_IMAGE,
				},
			},
			.bufferBindings = {
				{
					.slot = 2,
					.visibility = ShaderStage::COMPUTE,
					.type = BufferBindingType::UNIFORM,
				},
			},
		});

		m_ComputeVariant.shaderHashKey = Random::UInt64();

		m_EquirectToSkyboxShader = ResourceManager::Instance->CreateShader({
			.debugName = "compute-shader",
			.type = ShaderType::COMPUTE,
			.CS {.code = computeShaderCode[0], .entryPoint = "main" },
			.bindGroups {
				m_EquirectToSkyboxBindGroupLayout,							// (0)
			},
			.renderPipeline {
				.variants = { m_ComputeVariant },
			},
			.renderPass = m_PostProcessRenderPass,
		});

		// Skybox bind group.
		m_SkyboxBindGroupLayout = m_ResourceManager->CreateBindGroupLayout({
			.debugName = "skybox-global-layout",
			.textureBindings = {
				{
					.slot = 1,
					.visibility = ShaderStage::FRAGMENT,
					.type = TextureBindingType::IMAGE_SAMPLER,
				},
			},
		});

		// Global bind group
		m_SkyboxGlobalBindGroupLayout = m_ResourceManager->CreateBindGroupLayout({
			.debugName = "skybox-global-layout",
			.bufferBindings = {
				{
					.slot = 0,
					.visibility = ShaderStage::VERTEX,
					.type = BufferBindingType::UNIFORM,
				},
			},
		});

		auto cameraBuffer = m_ResourceManager->CreateBuffer({
			.debugName = "skybox-camera-uniform-buffer",
			.usage = BufferUsage::UNIFORM,
			.usageHint = BufferUsageHint::DYNAMIC,
			.memoryUsage = MemoryUsage::GPU_CPU,
			.byteSize = 64,
			.initialData = nullptr,
		});

		m_SkyboxGlobalBindGroup = m_ResourceManager->CreateBindGroup({
			.debugName = "skybox-global-bind-group",
			.layout = m_SkyboxGlobalBindGroupLayout,
			.buffers = {
				{.buffer = cameraBuffer },
			}
		});

		// Create skybox shader.
		const auto& skyboxShaderCode = ShaderUtilities::Get().Compile("assets/shaders/skybox.shader");

		m_SkyboxVariant.blend.enabled = false;
		m_SkyboxVariant.depthTest.writeEnabled = false;
		m_SkyboxVariant.depthTest.depthTest = Compare::LESS_OR_EQUAL;
		m_SkyboxVariant.cullMode = CullMode::FRONT;
		m_SkyboxVariant.shaderHashKey = Random::UInt64(); // Create a random UUID since we do not have an asset to retrieve from there the UUID.

		m_SkyboxShader = ResourceManager::Instance->CreateShader({
			.debugName = "skybox-shader",
			.VS {.code = skyboxShaderCode[0], .entryPoint = "main" },
			.FS {.code = skyboxShaderCode[1], .entryPoint = "main" },
			.bindGroups {
				m_SkyboxGlobalBindGroupLayout,	// Global bind group (0)
				m_SkyboxBindGroupLayout,		// (1)
			},
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = 12,
						.attributes = {
							{.byteOffset = 0, .format = VertexFormat::FLOAT32x3 },
						},
					}
				},
				.variants = { m_SkyboxVariant },
			},
			.renderPass = m_TransparentRenderPass,
		});

		// Cube mesh
		float vertexBuffer[] =
		{
			// Back face
			-1.0f,-1.0f,-1.0f, 1.0f, 1.0f,-1.0f, 1.0f,-1.0f,-1.0f,
			 1.0f, 1.0f,-1.0f,-1.0f,-1.0f,-1.0f,-1.0f, 1.0f,-1.0f,
			// Front face
			-1.0f,-1.0f, 1.0f, 1.0f,-1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			 1.0f, 1.0f, 1.0f,-1.0f, 1.0f, 1.0f,-1.0f,-1.0f, 1.0f,
		    // Left face
		    -1.0f, 1.0f, 1.0f,-1.0f, 1.0f,-1.0f,-1.0f,-1.0f,-1.0f,
		    -1.0f,-1.0f,-1.0f,-1.0f,-1.0f, 1.0f,-1.0f, 1.0f, 1.0f,
		    // Right face
		     1.0f, 1.0f, 1.0f, 1.0f,-1.0f,-1.0f, 1.0f, 1.0f,-1.0f,
		     1.0f,-1.0f,-1.0f, 1.0f, 1.0f, 1.0f, 1.0f,-1.0f, 1.0f,
		     // Bottom face
		    -1.0f,-1.0f,-1.0f, 1.0f,-1.0f,-1.0f, 1.0f,-1.0f, 1.0f,
		     1.0f,-1.0f, 1.0f,-1.0f,-1.0f, 1.0f,-1.0f,-1.0f,-1.0f,
		     // Top face
		    -1.0f, 1.0f,-1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,-1.0f,
		     1.0f, 1.0f, 1.0f,-1.0f, 1.0f,-1.0f,-1.0f, 1.0f, 1.0f,
		};

		m_CubeMeshBuffer = ResourceManager::Instance->CreateBuffer({
			.debugName = "cube-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.byteSize = sizeof(float) * 108,
			.initialData = vertexBuffer,
		});

		m_CubeMesh = ResourceManager::Instance->CreateMesh({
			.debugName = "cube-mesh",
			.meshes = {
				{
					.debugName = "cube-sub-mesh",
					.subMeshes = {
						{
							.vertexOffset = 0,
							.vertexCount = 36,
							.minVertex = { -1.0f, -1.0f, -1.0f },
							.maxVertex = {  1.0f,  1.0f,  1.0f },
						}
					},
					.vertexBuffers = { m_CubeMeshBuffer },
				}
			}
		});

		m_Scene->GetRegistry()
			.view<Component::SkyLight>()
			.each([&](Component::SkyLight& skyLight)
			{
				skyLight.CubeMapMaterial = {};
				skyLight.CubeMap = {};
				skyLight.Converted = false;
			});
	}

	void ForwardSceneRenderer::PostProcessPassSetup()
	{
		float* vertexBuffer = new float[24] {
			-1.0, -1.0, 0.0, 0.0, // Bottom left
			 1.0, -1.0, 1.0, 0.0, // Bottom right
			 1.0,  1.0, 1.0, 1.0, // Top right
			 1.0,  1.0, 1.0, 1.0, // Top right
			-1.0,  1.0, 0.0, 1.0, // Top left
			-1.0, -1.0, 0.0, 0.0  // Bottom left
		};

		m_PostProcessQuadVertexBuffer = m_ResourceManager->CreateBuffer({
			.debugName = "quad-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.byteSize = sizeof(float) * 24,
			.initialData = vertexBuffer,
		});

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
				{ .buffer = m_PostProcessBuffer },
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
					{.buffer = m_PostProcessBuffer },
				}
			});
		});

		// Create pre-pass shaders.
		const auto& postProcessShaderCode = ShaderUtilities::Get().Compile("assets/shaders/post-process-tone-mapping.shader");

		ShaderDescriptor::RenderPipeline::Variant variant = {};
		variant.blend.enabled = false;
		variant.depthTest.writeEnabled = false;
		variant.shaderHashKey = Random::UInt64(); // Create a random UUID since we do not have an asset to retrieve from there the UUID.
		variant.frontFace = FrontFace::CLOCKWISE;

		m_PostProcessShader = ResourceManager::Instance->CreateShader({
			.debugName = "post-process-shader",
			.VS { .code = postProcessShaderCode[0], .entryPoint = "main" },
			.FS { .code = postProcessShaderCode[1], .entryPoint = "main" },
			.bindGroups {
				m_PostProcessBindGroupLayout,	// Global bind group (0)
			},
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = 16,
						.attributes = {
							{ .byteOffset = 0, .format = VertexFormat::FLOAT32x2 },
							{ .byteOffset = 8, .format = VertexFormat::FLOAT32x2 },
						},
					}
				},
				.variants = { variant },
			},
			.renderPass = m_PostProcessRenderPass,
		});

		// Create post-process material.
		m_PostProcessMaterial = ResourceManager::Instance->CreateMaterial({
			.debugName = "post-process-material",
			.shader = m_PostProcessShader,
			.bindGroup = m_PostProcessBindGroup,
		});

		Material* mat = ResourceManager::Instance->GetMaterial(m_PostProcessMaterial);
		mat->VariantDescriptor = variant;
	}

	void ForwardSceneRenderer::DebugPassSetup()
	{
	}

	void ForwardSceneRenderer::PresentPassSetup()
	{
		float* vertexBuffer = new float[24] {
			-1.0, -1.0, 0.0, 1.0, // Bottom left
			 1.0, -1.0, 1.0, 1.0, // Bottom right
			 1.0,  1.0, 1.0, 0.0, // Top right
			 1.0,  1.0, 1.0, 0.0, // Top right
			-1.0,  1.0, 0.0, 0.0, // Top left
			-1.0, -1.0, 0.0, 1.0  // Bottom left
		};

		m_QuadVertexBuffer = m_ResourceManager->CreateBuffer({
			.debugName = "quad-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.byteSize = sizeof(float) * 24,
			.initialData = vertexBuffer,
		});

		ShaderDescriptor::RenderPipeline::Variant variant = {};
		variant.blend.enabled = false;
		variant.depthTest.enabled = false;
		variant.frontFace = FrontFace::CLOCKWISE;
		variant.shaderHashKey = Random::UInt64(); // Create a random UUID since we do not have an asset to retrieve from there the UUID.

		// Compile present shaders.
		const auto& presentShaderCode = ShaderUtilities::Get().Compile("assets/shaders/present.shader");

		// Create present bind group layout.
		m_PresentShader = ResourceManager::Instance->CreateShader({
			.debugName = "present-shader",
			.VS { .code = presentShaderCode[0], .entryPoint = "main" },
			.FS { .code = presentShaderCode[1], .entryPoint = "main" },
			.bindGroups {
				Renderer::Instance->GetGlobalPresentBindingsLayout(),	// Global bind group (0)
			},
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = 16,
						.attributes = {
							{ .byteOffset = 0, .format = VertexFormat::FLOAT32x2 },
							{ .byteOffset = 8, .format = VertexFormat::FLOAT32x2 },
						},
					}
				},
				.variants = { variant },
			},
			.renderPass = Renderer::Instance->GetMainRenderPass(),
		});

		m_QuadMaterial = m_ResourceManager->CreateMaterial({
			.debugName = "fullscreen-quad-material",
			.shader = m_PresentShader,
		});

		Material* mat = ResourceManager::Instance->GetMaterial(m_QuadMaterial);
		mat->VariantDescriptor = variant;
	}

	// Gathering.
	void ForwardSceneRenderer::GatherDraws(SceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		// Store the offset that the objects start from in the dynamic uniform buffer.
		sceneRenderData->m_UBOStartingOffset = m_UniformRingBuffer->GetCurrentOffset();

		// Static meshes
		{
			sceneRenderData->m_StaticMeshOpaqueDraws.Reset();
			sceneRenderData->m_StaticMeshTransparentDraws.Reset();
			sceneRenderData->m_PrePassStaticMeshDraws.Reset();
			sceneRenderData->m_ShadowPassStaticMeshDraws.Reset();

			m_Scene->Group<Component::StaticMesh>(Get<Component::Transform>)
				.Each([&](Component::StaticMesh& staticMesh, Component::Transform& transform)
				{
					if (staticMesh.Enabled)
					{
						if (!staticMesh.Material.IsValid() || !staticMesh.Mesh.IsValid())
						{
							return;
						}

						Material* material = ResourceManager::Instance->GetMaterial(staticMesh.Material);
						Mesh* mesh = ResourceManager::Instance->GetMesh(staticMesh.Mesh);
						const auto& meshPart = mesh->Meshes[staticMesh.MeshIndex];
						const auto& subMesh = meshPart.SubMeshes[staticMesh.SubMeshIndex];

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
							sceneRenderData->m_StaticMeshOpaqueDraws.Insert({
								.Shader = material->Shader,
								.Material = staticMesh.Material,
								.VariantHash = ResourceManager::Instance->GetShaderVariantHash(material->VariantDescriptor),
								.IndexBuffer = meshPart.IndexBuffer,
								.VertexBuffer = meshPart.VertexBuffers[0],
								.BindGroup = material->BindGroup,
								.Offset = alloc.Offset,
								.Size = sizeof(PerDrawData),
								.IndexCount = subMesh.IndexCount,
								.IndexOffset = subMesh.IndexOffset,
								.VertexCount = subMesh.VertexCount,
								.VertexOffset = subMesh.VertexOffset,
								.InstanceCount = subMesh.InstanceCount,
								.InstanceOffset = subMesh.InstanceOffset,
							});

							// Include only opaque objects in depth pre-pass.
							sceneRenderData->m_PrePassStaticMeshDraws.Insert({
								.Shader = m_DepthOnlyShader,
								.Material = m_DepthOnlyMaterial,
								.VariantHash = m_DepthOnlyMaterialHash,
								.IndexBuffer = meshPart.IndexBuffer,
								.VertexBuffer = meshPart.VertexBuffers[0],
								.BindGroup = m_DepthOnlyMeshBindGroup,
								.Offset = alloc.Offset,
								.Size = sizeof(PerDrawData),
								.IndexCount = subMesh.IndexCount,
								.IndexOffset = subMesh.IndexOffset,
								.VertexCount = subMesh.VertexCount,
								.VertexOffset = subMesh.VertexOffset,
								.InstanceCount = subMesh.InstanceCount,
								.InstanceOffset = subMesh.InstanceOffset,
							});
						}
						else
						{
							sceneRenderData->m_StaticMeshTransparentDraws.Insert({
								.Shader = material->Shader,
								.Material = staticMesh.Material,
								.VariantHash = ResourceManager::Instance->GetShaderVariantHash(material->VariantDescriptor),
								.IndexBuffer = meshPart.IndexBuffer,
								.VertexBuffer = meshPart.VertexBuffers[0],
								.BindGroup = material->BindGroup,
								.Offset = alloc.Offset,
								.Size = sizeof(PerDrawData),
								.IndexCount = subMesh.IndexCount,
								.IndexOffset = subMesh.IndexOffset,
								.VertexCount = subMesh.VertexCount,
								.VertexOffset = subMesh.VertexOffset,
								.InstanceCount = subMesh.InstanceCount,
								.InstanceOffset = subMesh.InstanceOffset,
							});
						}

						if (material->ReceiveShadows)
						{
							sceneRenderData->m_ShadowPassStaticMeshDraws.Insert({
								.Shader = m_ShadowPrePassShader,
								.Material = m_ShadowPrePassMaterial,
								.VariantHash = m_ShadowPrePassMaterialHash,
								.IndexBuffer = meshPart.IndexBuffer,
								.VertexBuffer = meshPart.VertexBuffers[0],
								.BindGroup = m_DepthOnlyMeshBindGroup,
								.Offset = alloc.Offset,
								.Size = sizeof(PerDrawData),
								.IndexCount = subMesh.IndexCount,
								.IndexOffset = subMesh.IndexOffset,
								.VertexCount = subMesh.VertexCount,
								.VertexOffset = subMesh.VertexOffset,
								.InstanceCount = subMesh.InstanceCount,
								.InstanceOffset = subMesh.InstanceOffset,
							});
						}
					}
				});
		}

		// Sprites
		{
			sceneRenderData->m_SpriteOpaqueDraws.Reset();
			sceneRenderData->m_SpriteTransparentDraws.Reset();
			sceneRenderData->m_PrePassSpriteDraws.Reset();

			m_Scene->Group<Component::Sprite>(Get<Component::Transform>)
				.Each([&](Component::Sprite& sprite, Component::Transform& transform)
				{
					if (sprite.Enabled)
					{
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
							sceneRenderData->m_SpriteOpaqueDraws.Insert({
								.Shader = material->Shader,
								.Material = sprite.Material,
								.VariantHash = ResourceManager::Instance->GetShaderVariantHash(material->VariantDescriptor),
								.VertexBuffer = m_VertexBuffer,
								.BindGroup = material->BindGroup,
								.Offset = alloc.Offset,
								.Size = sizeof(PerDrawDataSprite),
								.VertexCount = 6,
							});

							// Include only opaque objects in depth pre-pass.
							sceneRenderData->m_PrePassSpriteDraws.Insert({
								.Shader = m_DepthOnlySpriteShader,
								.Material = m_DepthOnlySpriteMaterial,
								.VariantHash = m_DepthOnlySpriteMaterialHash,
								.VertexBuffer = m_VertexBuffer,
								.BindGroup = m_DepthOnlySpriteBindGroup,
								.Offset = alloc.Offset,
								.Size = sizeof(PerDrawDataSprite),
								.VertexCount = 6,
							});
						}
						else
						{
							sceneRenderData->m_SpriteTransparentDraws.Insert({
								.Shader = material->Shader,
								.Material = sprite.Material,
								.VariantHash = ResourceManager::Instance->GetShaderVariantHash(material->VariantDescriptor),
								.VertexBuffer = m_VertexBuffer,
								.BindGroup = material->BindGroup,
								.Offset = alloc.Offset,
								.Size = sizeof(PerDrawDataSprite),
								.VertexCount = 6,
							});
						}
					}
				});
		}

		sceneRenderData->m_UBOEndingOffset = m_UniformRingBuffer->GetCurrentOffset();

		END_PROFILE_PASS(Renderer::Instance->GetStats().GatherTime);

		{
			BEGIN_PROFILE_PASS();

			sceneRenderData->m_StaticMeshOpaqueDraws.Sort();
			sceneRenderData->m_PrePassStaticMeshDraws.Sort();
			sceneRenderData->m_StaticMeshTransparentDraws.Sort();
			sceneRenderData->m_ShadowPassStaticMeshDraws.Sort();
			sceneRenderData->m_SpriteOpaqueDraws.Sort();
			sceneRenderData->m_PrePassSpriteDraws.Sort();
			sceneRenderData->m_SpriteTransparentDraws.Sort();

			END_PROFILE_PASS(Renderer::Instance->GetStats().SortingTime);
		}
	}

	void ForwardSceneRenderer::GatherLights(SceneRenderData* sceneRenderData)
	{
		sceneRenderData->m_LightData.LightCount = 0;

		m_Scene->Group<Component::Light>(Get<Component::Transform>)
			.Each([&](Component::Light& light, Component::Transform& transform)
			{
				if (light.Enabled)
				{
					float lightType = 0.0f;

					const Attenuation& attenuation = GetClosestAttenuation(light.Distance);

					int lightIndex = (int)sceneRenderData->m_LightData.LightCount;

					sceneRenderData->m_LightData.LightShadowData[lightIndex].x = light.CastsShadows ? 1.0f : 0.0f;

					switch (light.Type)
					{
					case Component::Light::EType::Directional:
						lightType = 0.0f;
						sceneRenderData->m_LightData.LightShadowData[lightIndex].y = light.ConstantBias;
						sceneRenderData->m_LightData.LightShadowData[lightIndex].z = light.SlopeBias;
						sceneRenderData->m_LightData.LightShadowData[lightIndex].w = light.NormalOffsetScale;
						break;
					case Component::Light::EType::Point:
						lightType = 1.0f;
						sceneRenderData->m_LightData.LightMetadata[lightIndex].y = attenuation.constant;
						sceneRenderData->m_LightData.LightMetadata[lightIndex].z = attenuation.linear;
						sceneRenderData->m_LightData.LightMetadata[lightIndex].w = attenuation.quadratic;

						sceneRenderData->m_LightData.LightShadowData[lightIndex].y = light.ConstantBias;
						sceneRenderData->m_LightData.LightShadowData[lightIndex].z = light.SlopeBias;
						sceneRenderData->m_LightData.LightShadowData[lightIndex].w = light.NormalOffsetScale;
						break;
					case Component::Light::EType::Spot:
						lightType = 2.0f;
						sceneRenderData->m_LightData.LightMetadata[lightIndex].y = glm::cos(glm::radians(light.InnerCutOff));
						sceneRenderData->m_LightData.LightMetadata[lightIndex].z = glm::cos(glm::radians(light.OuterCutOff));

						sceneRenderData->m_LightData.LightShadowData[lightIndex].y = light.ConstantBias;
						sceneRenderData->m_LightData.LightShadowData[lightIndex].z = light.SlopeBias;
						sceneRenderData->m_LightData.LightShadowData[lightIndex].w = light.NormalOffsetScale;
						break;
					}

					// Calculate light forward direction.
					glm::vec3 rotationRadians = glm::radians(transform.Rotation);
					glm::mat4 localRotation = glm::eulerAngleYXZ(rotationRadians.y, rotationRadians.x, rotationRadians.z);
					glm::vec3 localForward = glm::vec3(0.0f, -1.0f, 0.0f);
					glm::vec4 rotatedDirection = localRotation * glm::vec4(localForward, 0.0f); // w = 0 to avoid translation
					glm::mat3 worldRotation = glm::mat3(transform.WorldMatrix);// Extract rotation part of world matrix (3x3)
					glm::vec3 worldDirection = glm::normalize(worldRotation * glm::vec3(rotatedDirection));

					float near_plane = 0.1f, far_plane = 1000.0f;
					//glm::mat4 lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
					glm::mat4 lightProjection = glm::perspectiveRH_ZO(light.FieldOfView, 1.0f, near_plane, far_plane);

					glm::vec3 lightPos = glm::vec3(transform.WorldMatrix * glm::vec4(0.0, 0.0, 0.0, 1.0));
					glm::vec3 lightDir = normalize(worldDirection);
					glm::vec3 lightTarget = lightPos + lightDir; // look towards this point

					// Choose an up vector thats not colinear with your direction:
					glm::vec3 lightUp = glm::abs(glm::dot(lightDir, glm::vec3(0, 1, 0))) > 0.99f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);

					// If your light direction is nearly parallel to up, pick a different up vector:
					if (fabs(glm::dot(lightDir, lightUp)) > 0.99f)
					{
						lightUp = glm::vec3(1.0f, 0.0f, 0.0f);
					}

					glm::mat4 lightView = glm::lookAt(lightPos, lightTarget, lightUp);

					sceneRenderData->m_LightData.LightPositions[lightIndex] = transform.WorldMatrix * glm::vec4(transform.Translation, 1.0f);
					sceneRenderData->m_LightData.LightPositions[lightIndex].w = lightType;
					sceneRenderData->m_LightData.LightDirections[lightIndex] = glm::vec4(worldDirection, 0.0f);
					sceneRenderData->m_LightData.LightMetadata[lightIndex].x = light.Intensity;
					sceneRenderData->m_LightData.LightColors[lightIndex] = glm::vec4(light.Color, 1.0f);
					sceneRenderData->m_LightData.LightSpaceMatrices[lightIndex] = lightProjection * lightView;
					sceneRenderData->m_LightData.LightCount++;
				}
			});
	}

	// Pass rendering.
	void ForwardSceneRenderer::ShadowPass(CommandBuffer* commandBuffer, SceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		uint32_t index = 0;

		uint64_t uniformOffset = Device::Instance->GetGPUProperties().limits.minUniformBufferOffsetAlignment;
		uint32_t alignedSize = UniformRingBuffer::CeilToNextMultiple(sizeof(glm::mat4), uniformOffset);

		CreateAlignedMatrixArray(sceneRenderData, sceneRenderData->m_LightData.LightSpaceMatrices, 16, alignedSize);

		Handle<BindGroup> globalBindings = Renderer::Instance->GetShadowBindings();
		ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)sceneRenderData->m_LightSpaceMatricesData.data());

		m_Scene->Group<Component::Light>(Get<Component::Transform>)
			.Each([&](Component::Light& light, Component::Transform& transform)
			{
				if (light.Enabled)
				{
					if (light.CastsShadows)
					{
						ShadowTile tile = Renderer::Instance->ShadowAtlasAllocator.AllocateTile();

						if (tile == ShadowTile::Invalid)
						{
							return; // NOTE: Exceeded max number of shadow casting lights!
						}

						sceneRenderData->m_LightData.TileUVRange[index] = tile.GetUVRange();

						uint32_t tileX = tile.x * g_TileSize;
						uint32_t tileY = tile.y * g_TileSize;

						RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_ShadowRenderPass, m_ShadowFrameBuffer, { tileX, tileY, g_TileSize, g_TileSize });

						GlobalDrawStream globalDrawStream =
						{
							.BindGroup = globalBindings,
							.GlobalBufferSize = alignedSize,
							.GlobalBufferOffset = index * alignedSize,
							.UsesDynamicOffset = true,
						};
						passRenderer->DrawSubPass(globalDrawStream, sceneRenderData->m_ShadowPassStaticMeshDraws);

						commandBuffer->EndRenderPass(*passRenderer);
					}

					index++;
				}
			});

		Renderer::Instance->ShadowAtlasAllocator.Clear();

		END_PROFILE_PASS(Renderer::Instance->GetStats().ShadowPassTime);
	}

	void ForwardSceneRenderer::DepthPrePass(CommandBuffer* commandBuffer, SceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_DepthOnlyRenderPass, m_DepthOnlyFrameBuffer);

		Handle<BindGroup> globalBindings = Renderer::Instance->GetGlobalBindings2D();

		// Depth only pre pass for opaque static meshes.
		{
			ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&sceneRenderData->m_CameraData.ViewProjection);
			GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings, .UsesDynamicOffset = true };
			passRenderer->DrawSubPass(globalDrawStream, sceneRenderData->m_PrePassStaticMeshDraws);
		}

		// Depth only pre pass for opaque sprites.
		{
			ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&sceneRenderData->m_CameraData.ViewProjection);
			GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings, .UsesDynamicOffset = true };
			passRenderer->DrawSubPass(globalDrawStream, sceneRenderData->m_PrePassSpriteDraws);
		}

		commandBuffer->EndRenderPass(*passRenderer);

		END_PROFILE_PASS(Renderer::Instance->GetStats().PrePassTime);
	}

	void ForwardSceneRenderer::OpaquePass(CommandBuffer* commandBuffer, SceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_OpaqueRenderPass, m_OpaqueFrameBuffer);

		// Render opaque meshes.
		{
			Handle<BindGroup> globalBindings = Renderer::Instance->GetGlobalBindings3D();
			ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&sceneRenderData->m_CameraData);
			ResourceManager::Instance->SetBufferData(globalBindings, 1, (void*)&sceneRenderData->m_LightData);
			GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings, .UsesDynamicOffset = true };
			passRenderer->DrawSubPass(globalDrawStream, sceneRenderData->m_StaticMeshOpaqueDraws);
		}

		// Render opaque sprites.
		{
			Handle<BindGroup> globalBindings = Renderer::Instance->GetGlobalBindings2D();
			ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&sceneRenderData->m_CameraData.ViewProjection);
			GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings, .UsesDynamicOffset = true };
			passRenderer->DrawSubPass(globalDrawStream, sceneRenderData->m_SpriteOpaqueDraws);
		}

		commandBuffer->EndRenderPass(*passRenderer);

		END_PROFILE_PASS(Renderer::Instance->GetStats().OpaquePassTime);
	}

	void ForwardSceneRenderer::TransparentPass(CommandBuffer* commandBuffer, SceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_TransparentRenderPass, m_TransparentFrameBuffer);

		// Render transparent meshes.
		{
			Handle<BindGroup> globalBindings = Renderer::Instance->GetGlobalBindings3D();
			ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&sceneRenderData->m_CameraData);
			ResourceManager::Instance->SetBufferData(globalBindings, 1, (void*)&sceneRenderData->m_LightData);
			GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings, .UsesDynamicOffset = true };
			passRenderer->DrawSubPass(globalDrawStream, sceneRenderData->m_StaticMeshTransparentDraws);
		}

		// Render transparent sprites.
		{
			Handle<BindGroup> globalBindings = Renderer::Instance->GetGlobalBindings2D();
			ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&sceneRenderData->m_CameraData.ViewProjection);
			GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings, .UsesDynamicOffset = true };
			passRenderer->DrawSubPass(globalDrawStream, sceneRenderData->m_SpriteTransparentDraws);
		}

		commandBuffer->EndRenderPass(*passRenderer);

		END_PROFILE_PASS(Renderer::Instance->GetStats().TransparentPassTime);
	}

	void ForwardSceneRenderer::SkyboxPass(CommandBuffer* commandBuffer, SceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		DrawList draws;

		m_Scene->View<Component::SkyLight>()
			.Each([&](Component::SkyLight& skyLight)
			{
				if (skyLight.Enabled)
				{
					if (!skyLight.EquirectangularMap.IsValid())
					{
						return;
					}

					if (!skyLight.Converted)
					{
						if (skyLight.CubeMapMaterial.IsValid())
						{
							m_ResourceManager->DeleteTexture(skyLight.CubeMap);

							Material* mat = m_ResourceManager->GetMaterial(skyLight.CubeMapMaterial);
							m_ResourceManager->DeleteBindGroup(mat->BindGroup); 

							m_ResourceManager->DeleteMaterial(skyLight.CubeMapMaterial);

							m_ResourceManager->DeleteBindGroup(m_ComputeBindGroup);

							m_CaptureMatricesBuffer = m_ResourceManager->CreateBuffer({
								.debugName = "capture-matrices-buffer",
								.byteSize = sizeof(CaptureMatrices),
								.initialData = &g_CaptureMatrices,
							});
						}

						skyLight.CubeMap = m_ResourceManager->CreateTexture({
							.debugName = "skybox-texture",
							.dimensions = { (uint32_t)g_CaptureMatrices.FaceSize, (uint32_t)g_CaptureMatrices.FaceSize, 1 },
							.format = Format::RGBA16_FLOAT,
							.internalFormat = Format::RGBA16_FLOAT,
							.usage = { TextureUsage::TEXTURE_BINDING, TextureUsage::SAMPLED, TextureUsage::STORAGE_BINDING },
							.type = TextureType::CUBE,
							.aspect = TextureAspect::COLOR,
							.sampler = {.filter = Filter::LINEAR, .wrap = Wrap::CLAMP_TO_EDGE, },
							.initialLayout = TextureLayout::GENERAL,
						});

						ResourceManager::Instance->TransitionTextureLayout(
							commandBuffer,
							skyLight.CubeMap,
							TextureLayout::UNDEFINED,
							TextureLayout::GENERAL,
							{}
						);

						// FIXME: When entering the playmode multiple times in the session we create new descriptor sets in vk, so it exceeds the max in the pool.
						m_ComputeBindGroup = m_ResourceManager->CreateBindGroup({
							.debugName = "compute-bind-group",
							.layout = m_EquirectToSkyboxBindGroupLayout,
							.textures = { skyLight.EquirectangularMap, skyLight.CubeMap },
							.buffers = { { .buffer = m_CaptureMatricesBuffer, } }
						});

						Dispatch dispatch =
						{
							.Shader = m_EquirectToSkyboxShader,
							.BindGroup = m_ComputeBindGroup,
							.ThreadGroupCount = { (uint32_t)g_CaptureMatrices.FaceSize / 16, (uint32_t)g_CaptureMatrices.FaceSize / 16, 6 },
							.Variant = m_ComputeVariant,
						};

						ComputePassRenderer* computePassRenderer = commandBuffer->BeginComputePass({ skyLight.CubeMap }, {});
						computePassRenderer->Dispatch({ dispatch });
						commandBuffer->EndComputePass(*computePassRenderer);

						m_ResourceManager->DeleteBindGroup(m_ComputeBindGroup); // TODO: Fix this! Thats whats causing the validation error.

						auto skyboxBindGroup = m_ResourceManager->CreateBindGroup({
							.debugName = "skybox-bind-group",
							.layout = m_SkyboxBindGroupLayout,
							.textures = { skyLight.CubeMap }
						});

						// Create skybox material.
						skyLight.CubeMapMaterial = ResourceManager::Instance->CreateMaterial({
							.debugName = "skybox-material",
							.shader = m_SkyboxShader,
							.bindGroup = skyboxBindGroup,
						});

						Material* mat = ResourceManager::Instance->GetMaterial(skyLight.CubeMapMaterial);
						mat->VariantDescriptor = m_SkyboxVariant;

						skyLight.Converted = true;
					}

					if (!skyLight.CubeMapMaterial.IsValid())
					{
						return;
					}

					Material* mat = ResourceManager::Instance->GetMaterial(skyLight.CubeMapMaterial);

					draws.Insert({
						.Shader = m_SkyboxShader,
						.Material = skyLight.CubeMapMaterial,
						.VariantHash = ResourceManager::Instance->GetShaderVariantHash(mat->VariantDescriptor),
						.VertexBuffer = m_CubeMeshBuffer,
						.BindGroup = mat->BindGroup,
						.VertexCount = 36,
					});
				}
			});

		if (draws.GetCount() == 0)
		{
			return;
		}

		// Render Skybox
		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_TransparentRenderPass, m_TransparentFrameBuffer);

		ResourceManager::Instance->SetBufferData(m_SkyboxGlobalBindGroup, 0, (void*)&sceneRenderData->m_OnlyRotationInViewProjection);
		GlobalDrawStream globalDrawStream = { .BindGroup = m_SkyboxGlobalBindGroup };
		passRenderer->DrawSubPass(globalDrawStream, draws);

		commandBuffer->EndRenderPass(*passRenderer);

		END_PROFILE_PASS(Renderer::Instance->GetStats().SkyboxPassTime);
	}

	void ForwardSceneRenderer::PostProcessPass(CommandBuffer* commandBuffer, SceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		// Transition the layout of the texture that the scene is rendered to, in order to be sampled in the shader.
		ResourceManager::Instance->TransitionTextureLayout(
			commandBuffer,
			Renderer::Instance->IntermediateColorTexture,
			TextureLayout::RENDER_ATTACHMENT,
			TextureLayout::SHADER_READ_ONLY,
			m_PostProcessBindGroup);

		Material* mat = ResourceManager::Instance->GetMaterial(m_PostProcessMaterial);

		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_PostProcessRenderPass, m_PostProcessFrameBuffer);

		DrawList draws;
		draws.Insert({
			.Shader = m_PostProcessShader,
			.Material = m_PostProcessMaterial,
			.VariantHash = ResourceManager::Instance->GetShaderVariantHash(mat->VariantDescriptor),
			.VertexBuffer = m_PostProcessQuadVertexBuffer,
			.VertexCount = 6,
		});

		ResourceManager::Instance->SetBufferData(m_PostProcessBindGroup, 0, (void*)&sceneRenderData->m_CameraSettings);
		GlobalDrawStream globalDrawStream = { .BindGroup = m_PostProcessBindGroup };
		passRenderer->DrawSubPass(globalDrawStream, draws);

		commandBuffer->EndRenderPass(*passRenderer);

		END_PROFILE_PASS(Renderer::Instance->GetStats().PostProcessPassTime);
	}

	void ForwardSceneRenderer::DebugPass(CommandBuffer* commandBuffer, SceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		DebugRenderer::Instance->Render(commandBuffer);

		END_PROFILE_PASS(Renderer::Instance->GetStats().DebugPassTime);
	}

	void ForwardSceneRenderer::PresentPass(CommandBuffer* commandBuffer, SceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		// Transition the layout of the texture that the scene is rendered to, in order to be sampled in the shader.
		ResourceManager::Instance->TransitionTextureLayout(
			commandBuffer,
			Renderer::Instance->MainColorTexture,
			TextureLayout::RENDER_ATTACHMENT,
			TextureLayout::SHADER_READ_ONLY,
			Renderer::Instance->GetGlobalPresentBindings());

		Material* mat = ResourceManager::Instance->GetMaterial(m_QuadMaterial);

		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(Renderer::Instance->GetMainRenderPass(), Renderer::Instance->GetMainFrameBuffer());

		DrawList draws;
		draws.Insert({
			.Shader = m_PresentShader,
			.Material = m_QuadMaterial,
			.VariantHash = ResourceManager::Instance->GetShaderVariantHash(mat->VariantDescriptor),
			.VertexBuffer = m_QuadVertexBuffer,
			.VertexCount = 6,
		});

		GlobalDrawStream globalDrawStream = { .BindGroup = Renderer::Instance->GetGlobalPresentBindings() };
		passRenderer->DrawSubPass(globalDrawStream, draws);

		commandBuffer->EndRenderPass(*passRenderer);

		END_PROFILE_PASS(Renderer::Instance->GetStats().PresentPassTime);
	}

	void ForwardSceneRenderer::GetViewProjection(SceneRenderData* sceneRenderData, Entity mainCamera)
	{
		Scene* scene = (Context::Mode == Mode::Editor ? m_EditorScene : m_Scene);

		if (scene == nullptr || mainCamera == Entity::Null)
		{
			sceneRenderData->m_OnlyRotationInViewProjection = glm::mat4(1.0f);
			sceneRenderData->m_CameraData.ViewProjection = glm::mat4(1.0f);
			sceneRenderData->m_LightData.ViewPosition = glm::vec4(0.0f);
			sceneRenderData->m_CameraProjection = glm::mat4(1.0f);
			sceneRenderData->m_CameraSettings.Exposure = 1.0f;
			sceneRenderData->m_CameraSettings.Gamma = 2.2f;
			sceneRenderData->m_CameraFrustum = {};

			return;
		}

		Component::Camera& camera = scene->GetComponent<Component::Camera>(mainCamera);
		sceneRenderData->m_CameraSettings.Exposure = camera.Exposure;
		sceneRenderData->m_CameraSettings.Gamma = camera.Gamma;
		sceneRenderData->m_CameraData.ViewProjection = camera.ViewProjectionMatrix;
		sceneRenderData->m_CameraFrustum = camera.Frustum;

		Component::Transform& tr = scene->GetComponent<Component::Transform>(mainCamera);
		//m_LightData.ViewPosition = tr.WorldMatrix * glm::vec4(tr.Translation, 1.0f);
		sceneRenderData->m_LightData.ViewPosition = tr.WorldMatrix[3];
		sceneRenderData->m_OnlyRotationInViewProjection = camera.Projection * glm::mat4(glm::mat3(camera.View));
		sceneRenderData->m_CameraProjection = camera.Projection;
	}

	void ForwardSceneRenderer::CreateAlignedMatrixArray(SceneRenderData* sceneRenderData, const glm::mat4* matrices, size_t count, uint32_t alignedSize)
	{
		// Calculate total size
		size_t totalSize = alignedSize * count;
		sceneRenderData->m_LightSpaceMatricesData.resize(totalSize, 0);

		for (size_t i = 0; i < count; ++i)
		{
			size_t offset = i * alignedSize;
			std::memcpy(sceneRenderData->m_LightSpaceMatricesData.data() + offset, &matrices[i], sizeof(glm::mat4));
		}
	}
}
