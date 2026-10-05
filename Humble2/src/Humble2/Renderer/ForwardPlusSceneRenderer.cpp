#include "ForwardPlusSceneRenderer.h"

#include "Core/Window.h"
#include "Core/Context.h"
#include "Utilities/ShaderUtilities.h"

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

	struct DispatchParams
	{
		// Number of groups dispatched. (This parameter is not available as an Slang system value!)
		glm::u32vec3 numThreadGroups;
		uint32_t _padding0; // implicit padding to 16 bytes.

		// Total number of threads dispatched. (Also not available as an Slang system value!)
		// Note: This value may be less than the actual number of threads executed
		// if the screen size is not evenly divisible by the block size.
		glm::u32vec3 numThreads;
		uint32_t _padding1; // implicit padding to 16 bytes.
	};

	struct ScreenToViewParams
	{
		glm::mat4 InverseProjection;
		glm::vec2 ScreenDimensions;
		float LightCount;
		float _padding;
	};

	struct Plane
	{
		glm::vec3 N; // Plane normal.
		float d;  // Distance to origin.
	};

	struct Frustum
	{
		Plane planes[4]; // left, right, top, bottom frustum planes.
	};

	using packed_size = ShaderDescriptor::RenderPipeline::packed_size;

	void ForwardPlusSceneRenderer::Initialize(Scene* scene, uint32_t maxLights)
	{
		m_MaxLights = maxLights;
		uint32_t maxEntities = scene->GetDescriptor().maxEntities;

		// Calculate space needed for the 7 draw lists and 1 light buffer per frame in flight.
		uint64_t totalBytes = ArenaLayout::Create()
			.Add<LocalDrawStream>(Renderer::Instance->FrameCount * 7 * maxEntities)
			.Add<Light>(Renderer::Instance->FrameCount * maxLights)
			.Total();

		m_Reservation = Allocator::Arena.Reserve("ForwardSceneRendererPool", totalBytes);
		m_Arena.Initialize(&Allocator::Arena, totalBytes, m_Reservation);

		for (auto& sceneRenderData : m_RenderData)
		{
			sceneRenderData.m_LightData = FixedArray<Light>(&m_Arena, maxLights);
			sceneRenderData.m_LightData.resize(maxLights);

			sceneRenderData.m_PrePassSpriteDraws.Initialize(m_Arena, maxEntities);
			sceneRenderData.m_PrePassStaticMeshDraws.Initialize(m_Arena, maxEntities);
			sceneRenderData.m_ShadowPassStaticMeshDraws.Initialize(m_Arena, maxEntities);
			sceneRenderData.m_SpriteOpaqueDraws.Initialize(m_Arena, maxEntities);
			sceneRenderData.m_SpriteTransparentDraws.Initialize(m_Arena, maxEntities);
			sceneRenderData.m_StaticMeshOpaqueDraws.Initialize(m_Arena, maxEntities);
			sceneRenderData.m_StaticMeshTransparentDraws.Initialize(m_Arena, maxEntities);
		}

		m_Scene = scene;

		m_ResourceManager = ResourceManager::Instance;
		m_EditorScene = m_ResourceManager->GetScene(Context::EditorScene);
		m_UniformRingBuffer = Renderer::Instance->TempUniformRingBuffer;

		RenderPassSetup();
		BindingsSetup();

		// Setup render passes.
		ShadowPassSetup();
		DepthPrePassSetup();
		GridFrustumsComputePassSetup();
		LightCullingComputePassSetup();
		PostProcessPassSetup();
		PresentPassSetup();
	}

	void ForwardPlusSceneRenderer::Gather(Entity mainCamera)
	{
		ForwardPlusSceneRenderData* sceneRenderData = &m_RenderData[Renderer::Instance->GetFrameWriteIndex()];

		GetViewProjection(sceneRenderData, mainCamera);

		GatherDraws(sceneRenderData);
	}

	void ForwardPlusSceneRenderer::Render(void* renderData, void* debugRenderData)
	{
		BEGIN_PROFILE_PASS();

		ForwardPlusSceneRenderData* sceneRenderData = (ForwardPlusSceneRenderData*)renderData;
		ResourceManager* rm = ResourceManager::Instance;

		CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::MAIN);

		rm->TransitionTextureLayout(commandBuffer, Renderer::Instance->IntermediateColorTexture, TextureLayout::UNDEFINED, TextureLayout::RENDER_ATTACHMENT);
		rm->TransitionTextureLayout(commandBuffer, Renderer::Instance->MainColorTexture, TextureLayout::UNDEFINED, TextureLayout::RENDER_ATTACHMENT);
		rm->TransitionTextureLayout(commandBuffer, Renderer::Instance->ShadowAtlasTexture, TextureLayout::UNDEFINED, TextureLayout::DEPTH_STENCIL_ATTACHMENT);		

		ShadowPass(commandBuffer, sceneRenderData);		
		DepthPrePass(commandBuffer, sceneRenderData);
		rm->TransitionTextureLayout(commandBuffer, Renderer::Instance->MainDepthTexture, TextureLayout::DEPTH_STENCIL_ATTACHMENT, TextureLayout::DEPTH_STENCIL_READ_ONLY);
		GridFrustumsComputePass(commandBuffer, sceneRenderData);
		LightCullingComputePass(commandBuffer, sceneRenderData);

		// Geometry pass

		PostProcessPass(commandBuffer, sceneRenderData);

		// Debug pass

		PresentPass(commandBuffer, sceneRenderData);

		commandBuffer->EndCommandRecording();
		commandBuffer->Submit();

		END_PROFILE_PASS(Renderer::Instance->GetStats().MainPassTime);
	}

	void ForwardPlusSceneRenderer::CleanUp()
	{
		m_ResourceManager->DeleteRenderPassLayout(m_RenderPassLayout);

		// Shadow pass clean up.
		m_ResourceManager->DeleteTexture(m_ShadowDepthTexture);
		m_ResourceManager->DeleteRenderPass(m_ShadowRenderPass);
		m_ResourceManager->DeleteShader(m_ShadowPrePassShader);
		m_ResourceManager->DeleteMaterial(m_ShadowPrePassMaterial);

		// Depth pre pass clean up.
		m_ResourceManager->DeleteShader(m_DepthOnlyShader);
		m_ResourceManager->DeleteShader(m_DepthOnlySpriteShader);

		m_ResourceManager->DeleteBindGroup(m_DepthOnlyMeshBindGroup);
		m_ResourceManager->DeleteBindGroup(m_DepthOnlySpriteBindGroup);

		m_ResourceManager->DeleteMaterial(m_DepthOnlyMaterial);
		m_ResourceManager->DeleteMaterial(m_DepthOnlySpriteMaterial);

		m_ResourceManager->DeleteRenderPassLayout(m_DepthOnlyRenderPassLayout);
		m_ResourceManager->DeleteRenderPass(m_DepthOnlyRenderPass);
		Renderer::Instance->RemoveOnResizeCallback(std::string("Depth-Only-Resize-FrameBuffer-") + m_Scene->GetDescriptor().name.c_str());

		// Grid frustums compute pass clean up.
		m_ResourceManager->DeleteShader(m_GridFrustumsComputeShader);
		m_ResourceManager->DeleteBindGroup(m_GridFrustumsBindGroup);
		Renderer::Instance->RemoveOnResizeCallback(std::string("Grid-Frustums-BindGroup-Resize-") + m_Scene->GetDescriptor().name.c_str());

		// Light culling compute pass clean up.
		m_ResourceManager->DeleteShader(m_LightCullingComputeShader);
		m_ResourceManager->DeleteBindGroup(m_LightCullingBindGroup);

		// Post process pass clean up.
		m_ResourceManager->DeleteBuffer(m_PostProcessBuffer);
		m_ResourceManager->DeleteShader(m_PostProcessShader);
		m_ResourceManager->DeleteBindGroupLayout(m_PostProcessBindGroupLayout);
		m_ResourceManager->DeleteBindGroup(m_PostProcessBindGroup);
		m_ResourceManager->DeleteRenderPass(m_PostProcessRenderPass);
		Renderer::Instance->RemoveOnResizeCallback(std::string("Post-Process-Resize-FrameBuffer-") + m_Scene->GetDescriptor().name.c_str());
		m_ResourceManager->DeleteBuffer(m_PostProcessQuadVertexBuffer);

		// Present pass clean up.
		m_ResourceManager->DeleteBuffer(m_QuadVertexBuffer);
		m_ResourceManager->DeleteShader(m_PresentShader);
		m_ResourceManager->DeleteMaterial(m_QuadMaterial);

		m_Scene->Filter<Component::SkyLight>()
			.ForEach([&](Component::SkyLight& skyLight)
			{
				skyLight.EquirectangularMap.Release();
				skyLight.Converted = false;
			});

		m_Scene->Filter<Component::StaticMesh>()
			.ForEach([&](Component::StaticMesh& staticMesh)
			{
				staticMesh.Mesh.Release();
				staticMesh.Material.Release();
			});

		m_Scene->Filter<Component::Sprite>()
			.ForEach([&](Component::Sprite& sprite)
			{
				sprite.Material.Release();
			});

		// Scene renderer clean up.
		m_ShadowBindingsLayout.Release();
		m_GlobalBindingsLayout2D.Release();
		m_GlobalBindingsLayout3D.Release();

		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			m_ResourceManager->DeleteBindGroup(m_RenderData[i].ShadowBindings);
			m_ResourceManager->DeleteBindGroup(m_RenderData[i].GlobalBindings2D);
			m_ResourceManager->DeleteBindGroup(m_RenderData[i].GlobalBindings3D);
		}
	}

	void* ForwardPlusSceneRenderer::GetRenderData()
	{
		return &m_RenderData[Renderer::Instance->GetFrameWriteIndex()];
	}

	// Scene renderer set up.
	void ForwardPlusSceneRenderer::RenderPassSetup()
	{
		// Create color render pass.
		m_RenderPassLayout = m_ResourceManager->CreateRenderPassLayout({
			.debugName = "main-renderpass-layout",
			.depthTargetFormat = Format::D32_FLOAT,
			.subPasses = {
				{ .depthTarget = true, .colorTargets = 1, },
			},
		});

		// Create depth only render pass layout.
		m_DepthOnlyRenderPassLayout = m_ResourceManager->CreateRenderPassLayout({
			.debugName = "pre-pass-renderpass-layout",
			.depthTargetFormat = Format::D32_FLOAT,
			.subPasses = {
				{ .depthTarget = true },
			},
		});
	}
	
	void ForwardPlusSceneRenderer::BindingsSetup()
	{
		// Global bindings layout for the 2D rendering.
		m_GlobalBindingsLayout2D = ResourceManager::Instance->CreateBindGroupLayout({
			.debugName = "global-bind-group-layout-2d",
			.bufferBindings = {
				{
					.slot = 0,
					.visibility = { ShaderStage::VERTEX },
					.type = BufferBindingType::UNIFORM,
				},
			},
		});

		// Global bindings layout for the 3D rendering.
		m_GlobalBindingsLayout3D = ResourceManager::Instance->CreateBindGroupLayout({
			.debugName = "global-bind-group-layout-3d",
			.textureBindings = {
				{
					.slot = 2,
					.visibility = { ShaderStage::FRAGMENT },
				},
			},
			.bufferBindings = {
				{
					.slot = 0,
					.visibility = { ShaderStage::VERTEX, ShaderStage::FRAGMENT },
					.type = BufferBindingType::UNIFORM,
				},
				{
					.slot = 1,
					.visibility = { ShaderStage::FRAGMENT },
					.type = BufferBindingType::STORAGE,
				},
			},
		});

		// Bindings layout for shadow rendering.
		m_ShadowBindingsLayout = ResourceManager::Instance->CreateBindGroupLayout({
			.debugName = "shadow-bindings-layout",
			.bufferBindings = {
				{
					.slot = 0,
					.visibility = { ShaderStage::VERTEX },
					.type = BufferBindingType::UNIFORM,
				},
			},
		});

		// Global bindings for the 2D rendering.
		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			auto cameraBuffer2D = m_ResourceManager->CreateBuffer({
				.debugName = "camera-uniform-buffer",
				.usage = BufferUsage::UNIFORM,
				.memoryUsage = MemoryUsage::CPU_GPU,
				.byteSize = 64,
				.initialData = nullptr,
			});

			m_RenderData[i].GlobalBindings2D = m_ResourceManager->CreateBindGroup({
				.debugName = "unlit-colored-bind-group",
				.layout = m_GlobalBindingsLayout2D.Get(),
				.buffers = {
					{ .buffer = cameraBuffer2D },
				}
			});
		}

		// Create pre-pass bind groups.
		m_DepthOnlyMeshBindGroup = ResourceManager::Instance->CreateBindGroup({
			.debugName = "pre-pass-mesh-bind-group",
			.layout = Renderer::Instance->GetDynamicBindingsLayout(),
			.buffers = {
				{ .buffer = Renderer::Instance->TempUniformRingBuffer->GetBuffer(), .range = sizeof(PerDrawData) },
			}
		});

		m_DepthOnlySpriteBindGroup = ResourceManager::Instance->CreateBindGroup({
			.debugName = "pre-pass-sprite-bind-group",
			.layout = Renderer::Instance->GetDynamicBindingsLayout(),
			.buffers = {
				{ .buffer = Renderer::Instance->TempUniformRingBuffer->GetBuffer(), .range = sizeof(PerDrawDataSprite) },
			}
		});
	}

	// Pass set up.
	void ForwardPlusSceneRenderer::ShadowPassSetup()
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
				.prevUsage = TextureLayout::UNDEFINED,
				.nextUsage = TextureLayout::DEPTH_STENCIL_ATTACHMENT,
			},
			.frameBufferDesc = {
				.width = g_ShadowAtlasSize,
				.height = g_ShadowAtlasSize,
				.depthTarget = Renderer::Instance->ShadowAtlasTexture,
			}
		});

		// Create shadow pre-pass shader.
		const auto& shadowPrePassShaderData = ShaderUtilities::Get().Compile("assets/shaders/shadow-mapping-pre-pass.slang", (ShaderReflectionData*)nullptr, false);

		ShaderDescriptor::RenderPipeline::PackedVariant variant = {};
		variant.colorOutput = false;
		variant.blendEnabled = false;
		variant.depthCompare = (packed_size)Compare::LESS;
		variant.depthWrite = true;
		variant.cullMode = (packed_size)CullMode::FRONT;

		m_ShadowPrePassShader = ResourceManager::Instance->CreateShader({
			.debugName = "shadow-pre-pass-shader",
			.VS { .code = shadowPrePassShaderData.vertexShaderCode.AsSpan(), .entryPoint = "mainVS" },
			.FS { .code = shadowPrePassShaderData.fragmentShaderCode.AsSpan(), .entryPoint = "mainPS" },
			.bindGroups {
				m_ShadowBindingsLayout.Get(),					// Global bind group (0)
				Renderer::Instance->GetEmptyBindingsLayout(),	// Unused (1)
				Renderer::Instance->GetEmptyBindingsLayout(),	// Unused (2)
				Renderer::Instance->GetDynamicBindingsLayout(), // (3)
			},
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = 32,
						.attributes = {
							{.byteOffset = 0,  .format = VertexFormat::FLOAT32x3 },
							{.byteOffset = 12, .format = VertexFormat::FLOAT32x3 },
							{.byteOffset = 24, .format = VertexFormat::FLOAT32x2 },
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
			.drawBindGroup = m_DepthOnlyMeshBindGroup,
		});

		Material* mat = ResourceManager::Instance->GetMaterial(m_ShadowPrePassMaterial);
		mat->VariantHash = variant;

		m_ShadowPrePassMaterialHash = variant;
	}

	void ForwardPlusSceneRenderer::DepthPrePassSetup()
	{
		// Create pre-pass framebuffer.
		m_DepthOnlyRenderPass = m_ResourceManager->CreateRenderPass({
			.debugName = "pre-pass-renderpass",
			.layout = m_DepthOnlyRenderPassLayout,
			.depthTarget = {
				.loadOp = LoadOperation::CLEAR,
				.storeOp = StoreOperation::STORE,
				.stencilLoadOp = LoadOperation::DONT_CARE,
				.stencilStoreOp = StoreOperation::DONT_CARE,
				.prevUsage = TextureLayout::UNDEFINED,
				.nextUsage = TextureLayout::DEPTH_STENCIL_ATTACHMENT,
			},
			.frameBufferDesc = {
				.width = Window::Instance->GetExtents().x,
				.height = Window::Instance->GetExtents().y,
				.depthTarget = Renderer::Instance->MainDepthTexture,
			}
		});

		Renderer::Instance->AddCallbackOnResize(std::string("Depth-Only-Resize-FrameBuffer-") + m_Scene->GetDescriptor().name.c_str(), [this](uint32_t width, uint32_t height)
		{
			ResourceManager::Instance->RecreateRenderPassFrameBuffer(m_DepthOnlyRenderPass, {
				.width = width,
				.height = height,
				.depthTarget = Renderer::Instance->MainDepthTexture,
			});
		});

		// Create pre-pass shaders.
		const auto& prePassShaderData = ShaderUtilities::Get().Compile("assets/shaders/depth-pre-pass-mesh.slang", (ShaderReflectionData*)nullptr, false);

		ShaderDescriptor::RenderPipeline::PackedVariant variant = {};
		variant.colorOutput = false;
		variant.blendEnabled = false;
		variant.depthCompare = (packed_size)Compare::LESS;
		variant.depthWrite = true;

		m_DepthOnlyShader = ResourceManager::Instance->CreateShader({
			.debugName = "mesh-pre-pass-shader",
			.VS {.code = prePassShaderData.vertexShaderCode.AsSpan(), .entryPoint = "mainVS" },
			.FS {.code = prePassShaderData.fragmentShaderCode.AsSpan(), .entryPoint = "mainPS" },
			.bindGroups {
				m_GlobalBindingsLayout2D.Get(),						// Global bind group (0)
				Renderer::Instance->GetEmptyBindingsLayout(),		// Unused (1)
				Renderer::Instance->GetEmptyBindingsLayout(),		// Unused (2)
				Renderer::Instance->GetDynamicBindingsLayout(),		// (3)
			},
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = 32,
						.attributes = {
							{.byteOffset = 0,  .format = VertexFormat::FLOAT32x3 },
							{.byteOffset = 12, .format = VertexFormat::FLOAT32x3 },
							{.byteOffset = 24, .format = VertexFormat::FLOAT32x2 },
						},
					}
				},
				.variants = { variant },
			},
			.renderPass = m_DepthOnlyRenderPass,
		});

		const auto& prePassSpriteShaderData = ShaderUtilities::Get().Compile("assets/shaders/depth-pre-pass-sprite.slang", (ShaderReflectionData*)nullptr, false);

		m_DepthOnlySpriteShader = ResourceManager::Instance->CreateShader({
			.debugName = "sprite-pre-pass-shader",
			.VS {.code = prePassSpriteShaderData.vertexShaderCode.AsSpan(), .entryPoint = "mainVS" },
			.FS {.code = prePassSpriteShaderData.fragmentShaderCode.AsSpan(), .entryPoint = "mainPS" },
			.bindGroups {
				m_GlobalBindingsLayout2D.Get(),						// Global bind group (0)
				Renderer::Instance->GetEmptyBindingsLayout(),		// Unused (1)
				Renderer::Instance->GetEmptyBindingsLayout(),		// Unused (2)
				Renderer::Instance->GetDynamicBindingsLayout(),		// (3)
			},
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = 20,
						.attributes = {
							{.byteOffset = 0,  .format = VertexFormat::FLOAT32x3 },
							{.byteOffset = 12, .format = VertexFormat::FLOAT32x2 },
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
			.drawBindGroup = m_DepthOnlyMeshBindGroup,
		});

		Material* mat0 = ResourceManager::Instance->GetMaterial(m_DepthOnlyMaterial);
		mat0->VariantHash = variant;

		m_DepthOnlySpriteMaterial = ResourceManager::Instance->CreateMaterial({
			.debugName = "depth-only-sprite-material",
			.shader = m_DepthOnlySpriteShader,
			.drawBindGroup = m_DepthOnlySpriteBindGroup,
		});

		Material* mat1 = ResourceManager::Instance->GetMaterial(m_DepthOnlySpriteMaterial);
		mat1->VariantHash = variant;

		m_DepthOnlyMaterialHash = variant;
		m_DepthOnlySpriteMaterialHash = m_DepthOnlyMaterialHash;
	}

	void ForwardPlusSceneRenderer::GridFrustumsComputePassSetup()
	{
		// Create compute bind group layout.
		m_GridFrustumsBindGroupLayout = ResourceManager::Instance->CreateBindGroupLayout({
			.debugName = "grid-frustums-bind-group-layout",
			.bufferBindings = {
				{ .slot = 0, .visibility = ShaderStage::COMPUTE, .type = BufferBindingType::UNIFORM },
				{ .slot = 1, .visibility = ShaderStage::COMPUTE, .type = BufferBindingType::UNIFORM },
				{ .slot = 2, .visibility = ShaderStage::COMPUTE, .type = BufferBindingType::STORAGE }
			}
		});

		// Compile compute shader.
		const auto& compilationData = ShaderUtilities::Get().Compile("assets/shaders/fwdp-grid-frustums.slang", nullptr, false);

		m_GridFrustumsComputeShader = ResourceManager::Instance->CreateShader({
			.debugName = "grid-frustums-compute-shader",
			.type = ShaderType::COMPUTE,
			.CS { .code = compilationData.computeShaderCode.AsSpan(), .entryPoint = "mainCS" },
			.bindGroups {
				m_GridFrustumsBindGroupLayout,	// (0)
			},
			.renderPipeline {
				.variants = { m_GridFrustumsComputeVariant },
			},
			.threadsPerThreadGroup = { 16, 16, 1 },
		});

		Renderer::Instance->AddCallbackOnResize(std::string("Grid-Frustums-BindGroup-Resize-") + m_Scene->GetDescriptor().name.c_str(), [this](uint32_t width, uint32_t height)
		{
			// Delete and invalidate old bind group.
			ResourceManager::Instance->DeleteBindGroup(m_GridFrustumsBindGroup);
			m_GridFrustumsBindGroup = {};
		});
	}

	void ForwardPlusSceneRenderer::LightCullingComputePassSetup()
	{
		// Create compute bind group layout.
		m_LightCullingBindGroupLayout = ResourceManager::Instance->CreateBindGroupLayout({
			.debugName = "light-culling-bind-group-layout",
			.textureBindings = {
				{ .slot = 2,  .visibility = ShaderStage::COMPUTE, .type = TextureBindingType::IMAGE_SAMPLER },
				{ .slot = 9,  .visibility = ShaderStage::COMPUTE, .type = TextureBindingType::STORAGE_IMAGE },
				{ .slot = 10, .visibility = ShaderStage::COMPUTE, .type = TextureBindingType::STORAGE_IMAGE },
			},
			.bufferBindings = {
				{ .slot = 0, .visibility = ShaderStage::COMPUTE, .type = BufferBindingType::UNIFORM },
				{ .slot = 1, .visibility = ShaderStage::COMPUTE, .type = BufferBindingType::UNIFORM },
				{ .slot = 3, .visibility = ShaderStage::COMPUTE, .type = BufferBindingType::READ_ONLY_STORAGE },
				{ .slot = 4, .visibility = ShaderStage::COMPUTE, .type = BufferBindingType::READ_ONLY_STORAGE },
				{ .slot = 5, .visibility = ShaderStage::COMPUTE, .type = BufferBindingType::STORAGE },
				{ .slot = 6, .visibility = ShaderStage::COMPUTE, .type = BufferBindingType::STORAGE },
				{ .slot = 7, .visibility = ShaderStage::COMPUTE, .type = BufferBindingType::STORAGE },
				{ .slot = 8, .visibility = ShaderStage::COMPUTE, .type = BufferBindingType::STORAGE },
			}
		});

		// Compile compute shader.
		const auto& compilationData = ShaderUtilities::Get().Compile("assets/shaders/fwdp-light-culling.slang", nullptr, false);

		m_LightCullingComputeShader = ResourceManager::Instance->CreateShader({
			.debugName = "light-culling-compute-shader",
			.type = ShaderType::COMPUTE,
			.CS { .code = compilationData.computeShaderCode.AsSpan(), .entryPoint = "mainCS" },
			.bindGroups {
				m_LightCullingBindGroupLayout,	// (0)
			},
			.renderPipeline {
				.variants = { m_LightCullingComputeVariant },
			},
			.threadsPerThreadGroup = { 16, 16, 1 },
		});
	}

	void ForwardPlusSceneRenderer::PostProcessPassSetup()
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
			.byteSize = sizeof(CameraSettings),
		});

		// Create post-process bind group.
		m_PostProcessBindGroupLayout = m_ResourceManager->CreateBindGroupLayout({
			.debugName = "post-process-bind-group-layout",
			.textureBindings = {
				{
					.slot = 0,
					.visibility = { ShaderStage::FRAGMENT },
				},
			},
			.bufferBindings = {
				{
					.slot = 1,
					.visibility = { ShaderStage::FRAGMENT },
					.type = BufferBindingType::UNIFORM,
				},
			},
		});

		m_PostProcessBindGroup = ResourceManager::Instance->CreateBindGroup({
			.debugName = "post-process-bind-group",
			.layout = m_PostProcessBindGroupLayout,
			.textures = {
				{ Renderer::Instance->IntermediateColorTexture, TextureLayout::SHADER_READ_ONLY }
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
				.prevUsage = TextureLayout::DEPTH_STENCIL_READ_ONLY,
				.nextUsage = TextureLayout::DEPTH_STENCIL_READ_ONLY,
			},
			.colorTargets = {
				{
					.loadOp = LoadOperation::CLEAR,
					.storeOp = StoreOperation::STORE,
					.prevUsage = TextureLayout::UNDEFINED,
					.nextUsage = TextureLayout::RENDER_ATTACHMENT,
				},
			},
			.frameBufferDesc = {
				.width = Window::Instance->GetExtents().x,
				.height = Window::Instance->GetExtents().y,
				.depthTarget = Renderer::Instance->MainDepthTexture,
				.colorTargets = { Renderer::Instance->MainColorTexture },
			}
		});

		Renderer::Instance->AddCallbackOnResize(std::string("Post-Process-Resize-FrameBuffer-") + m_Scene->GetDescriptor().name.c_str(), [this](uint32_t width, uint32_t height)
		{
			ResourceManager::Instance->RecreateRenderPassFrameBuffer(m_PostProcessRenderPass, {
				.width = width,
				.height = height,
				.depthTarget = Renderer::Instance->MainDepthTexture,
				.colorTargets = { Renderer::Instance->MainColorTexture },
			});

			ResourceManager::Instance->DeleteBindGroup(m_PostProcessBindGroup);

			m_PostProcessBuffer = m_ResourceManager->CreateBuffer({
				.debugName = "camera-settings-buffer",
				.usage = BufferUsage::UNIFORM,
				.byteSize = sizeof(CameraSettings),
			});

			m_PostProcessBindGroup = ResourceManager::Instance->CreateBindGroup({
				.debugName = "post-process-bind-group",
				.layout = m_PostProcessBindGroupLayout,
				.textures = {
					{ Renderer::Instance->IntermediateColorTexture, TextureLayout::SHADER_READ_ONLY }
				},
				.buffers = {
					{.buffer = m_PostProcessBuffer },
				}
			});
		});

		// Create pre-pass shaders.
		const auto& postProcessShaderData = ShaderUtilities::Get().Compile("assets/shaders/post-process-tone-mapping.slang", (ShaderReflectionData*)nullptr, false);

		ShaderDescriptor::RenderPipeline::PackedVariant variant = {};
		variant.blendEnabled = false;
		variant.depthWrite = false;
		variant.frontFace = (packed_size)FrontFace::CLOCKWISE;

		m_PostProcessShader = ResourceManager::Instance->CreateShader({
			.debugName = "post-process-shader",
			.VS {.code = postProcessShaderData.vertexShaderCode.AsSpan(), .entryPoint = "mainVS" },
			.FS {.code = postProcessShaderData.fragmentShaderCode.AsSpan(), .entryPoint = "mainPS" },
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

		// Cache post-process variant hash.
		m_PostProcessShaderVariantHash = variant;
	}

	void ForwardPlusSceneRenderer::PresentPassSetup()
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

		ShaderDescriptor::RenderPipeline::PackedVariant variant = {};
		variant.blendEnabled = false;
		variant.depthEnabled = false;
		variant.depthWrite = true;
		variant.frontFace = (packed_size)FrontFace::CLOCKWISE;

		// Compile present shaders.
		const auto& presentShaderData = ShaderUtilities::Get().Compile("assets/shaders/present.slang", (ShaderReflectionData*)nullptr, false);

		// Create present bind group layout.
		m_PresentShader = ResourceManager::Instance->CreateShader({
			.debugName = "present-shader",
			.VS {.code = presentShaderData.vertexShaderCode.AsSpan(), .entryPoint = "mainVS" },
			.FS {.code = presentShaderData.fragmentShaderCode.AsSpan(), .entryPoint = "mainPS" },
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
		mat->VariantHash = variant;
	}
	
	// Gathering.
	void ForwardPlusSceneRenderer::GatherDraws(ForwardPlusSceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		// Store the offset that the objects start from in the dynamic uniform buffer.
		sceneRenderData->m_UBOStartingOffset = m_UniformRingBuffer->GetCurrentOffset();

		// Static meshes
		{
			uint64_t depthOnlyVariantHandle = ResourceManager::Instance->GetOrAddShaderVariant(m_DepthOnlyShader, m_DepthOnlyMaterialHash);
			// uint64_t shadowPrePassVariantHandle = ResourceManager::Instance->GetOrAddShaderVariant(m_ShadowPrePassShader, m_ShadowPrePassMaterialHash);

			sceneRenderData->m_StaticMeshOpaqueDraws.Reset();
			sceneRenderData->m_StaticMeshTransparentDraws.Reset();
			sceneRenderData->m_PrePassStaticMeshDraws.Reset();
			sceneRenderData->m_ShadowPassStaticMeshDraws.Reset();

			m_Scene->Filter<Component::StaticMesh, Component::Transform>()
				.ForEach([&](Component::StaticMesh& staticMesh, Component::Transform& transform)
				{
					if (staticMesh.Enabled)
					{
						if (!staticMesh.Material.IsValid() || !staticMesh.Mesh.IsValid())
						{
							return;
						}

						Handle<Material> materialHandle = AssetManager::Instance->GetAsset<Material>(staticMesh.Material.Get());
						Material* material = ResourceManager::Instance->GetMaterial(materialHandle);

						if (material == nullptr)
						{
							return;
						}

						Handle<Mesh> meshHandle = AssetManager::Instance->GetAsset<Mesh>(staticMesh.Mesh.Get());
						Mesh* mesh = ResourceManager::Instance->GetMesh(meshHandle);

						if (mesh == nullptr || mesh->IsEmpty())
						{
							return;
						}

						const auto& meshPart = mesh->Meshes[staticMesh.MeshIndex];

						if (meshPart.IsEmpty())
						{
							return;
						}

						const auto& subMesh = meshPart.SubMeshes[staticMesh.SubMeshIndex];

						// Bump allocate and set per draw data.
						auto alloc = m_UniformRingBuffer->BumpAllocate<PerDrawData>();
						alloc.Data->Model = transform.WorldMatrix;
						alloc.Data->InverseModel = glm::transpose(glm::inverse(glm::mat3(transform.WorldMatrix)));
						alloc.Data->Color = glm::vec4(1.0f);

						// Fill draw lists.
						if (!material->VariantHash.blendEnabled)
						{
							sceneRenderData->m_StaticMeshOpaqueDraws.Insert({
								.Shader = material->Shader,
								.VariantHandle = ResourceManager::Instance->GetOrAddShaderVariant(material->Shader, material->VariantHash),
								.IndexBuffer = meshPart.IndexBuffer,
								.VertexBuffer = meshPart.VertexBuffers[0],
								.MaterialBindGroup = material->MaterialBindGroup.Get(),
								.BindGroup = material->DrawBindGroup.Get(),
								.Size = sizeof(PerDrawData),
								.Offset = alloc.Offset,
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
								.VariantHandle = depthOnlyVariantHandle,
								.IndexBuffer = meshPart.IndexBuffer,
								.VertexBuffer = meshPart.VertexBuffers[0],
								.MaterialBindGroup = Renderer::Instance->GetEmptyBindings(),
								.BindGroup = m_DepthOnlyMeshBindGroup,
								.Size = sizeof(PerDrawData),
								.Offset = alloc.Offset,
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
								.VariantHandle = ResourceManager::Instance->GetOrAddShaderVariant(material->Shader, material->VariantHash),
								.IndexBuffer = meshPart.IndexBuffer,
								.VertexBuffer = meshPart.VertexBuffers[0],
								.MaterialBindGroup = material->MaterialBindGroup.Get(),
								.BindGroup = material->DrawBindGroup.Get(),
								.Size = sizeof(PerDrawData),
								.Offset = alloc.Offset,
								.IndexCount = subMesh.IndexCount,
								.IndexOffset = subMesh.IndexOffset,
								.VertexCount = subMesh.VertexCount,
								.VertexOffset = subMesh.VertexOffset,
								.InstanceCount = subMesh.InstanceCount,
								.InstanceOffset = subMesh.InstanceOffset,
							});
						}

						/*if (material->ReceiveShadows)
						{
							sceneRenderData->m_ShadowPassStaticMeshDraws.Insert({
								.Shader = m_ShadowPrePassShader,
								.VariantHandle = shadowPrePassVariantHandle,
								.IndexBuffer = meshPart.IndexBuffer,
								.VertexBuffer = meshPart.VertexBuffers[0],
								.MaterialBindGroup = Renderer::Instance->GetEmptyBindings(),
								.BindGroup = m_DepthOnlyMeshBindGroup,
								.Size = sizeof(PerDrawData),
								.Offset = alloc.Offset,
								.IndexCount = subMesh.IndexCount,
								.IndexOffset = subMesh.IndexOffset,
								.VertexCount = subMesh.VertexCount,
								.VertexOffset = subMesh.VertexOffset,
								.InstanceCount = subMesh.InstanceCount,
								.InstanceOffset = subMesh.InstanceOffset,
							});
						}*/
					}
				});
		}

		// Sprites
		{
			uint64_t depthOnlySpriteVariantHandle = ResourceManager::Instance->GetOrAddShaderVariant(m_DepthOnlySpriteShader, m_DepthOnlySpriteMaterialHash);

			sceneRenderData->m_SpriteOpaqueDraws.Reset();
			sceneRenderData->m_SpriteTransparentDraws.Reset();
			sceneRenderData->m_PrePassSpriteDraws.Reset();

			m_Scene->Filter<Component::Sprite, Component::Transform>()
				.ForEach([&](Component::Sprite& sprite, Component::Transform& transform)
				{
					if (sprite.Enabled)
					{
						if (!sprite.Material.IsValid())
						{
							return;
						}

						Handle<Material> materialHandle = AssetManager::Instance->GetAsset<Material>(sprite.Material.Get());
						Material* material = ResourceManager::Instance->GetMaterial(materialHandle);

						if (material == nullptr)
						{
							return;
						}

						// Bump allocate and set per draw data.
						auto alloc = m_UniformRingBuffer->BumpAllocate<PerDrawDataSprite>();
						alloc.Data->Model = transform.WorldMatrix;
						alloc.Data->Color = glm::vec4(1.0f);

						// Fill draw lists.
						if (!material->VariantHash.blendEnabled)
						{
							sceneRenderData->m_SpriteOpaqueDraws.Insert({
								.Shader = material->Shader,
								.VariantHandle = ResourceManager::Instance->GetOrAddShaderVariant(material->Shader, material->VariantHash),
								.VertexBuffer = m_VertexBuffer,
								.MaterialBindGroup = material->MaterialBindGroup.Get(),
								.BindGroup = material->DrawBindGroup.Get(),
								.Size = sizeof(PerDrawDataSprite),
								.Offset = alloc.Offset,
								.VertexCount = 6,
							});

							// Include only opaque objects in depth pre-pass.
							sceneRenderData->m_PrePassSpriteDraws.Insert({
								.Shader = m_DepthOnlySpriteShader,
								.VariantHandle = depthOnlySpriteVariantHandle,
								.VertexBuffer = m_VertexBuffer,
								.MaterialBindGroup = Renderer::Instance->GetEmptyBindings(),
								.BindGroup = m_DepthOnlySpriteBindGroup,
								.Size = sizeof(PerDrawDataSprite),
								.Offset = alloc.Offset,
								.VertexCount = 6,
							});
						}
						else
						{
							sceneRenderData->m_SpriteTransparentDraws.Insert({
								.Shader = material->Shader,
								.VariantHandle = ResourceManager::Instance->GetOrAddShaderVariant(material->Shader, material->VariantHash),
								.VertexBuffer = m_VertexBuffer,
								.MaterialBindGroup = material->MaterialBindGroup.Get(),
								.BindGroup = material->DrawBindGroup.Get(),
								.Size = sizeof(PerDrawDataSprite),
								.Offset = alloc.Offset,
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

	void ForwardPlusSceneRenderer::GatherLights(ForwardPlusSceneRenderData* sceneRenderData)
	{
		int lightIndex = 0;
		m_Scene->Filter<Component::Light, Component::Transform>()
			.ForEach([&](Component::Light& light, Component::Transform& transform)
			{
				if (light.Enabled)
				{
					Light& data = sceneRenderData->m_LightData[lightIndex];
					data = {};

					float lightType = 0.0f;
					float lightRange = 1.0f;

					const Attenuation& attenuation = GetClosestAttenuation(light.Distance);

					sceneRenderData->m_LightData[lightIndex].LightShadowData.x = light.CastsShadows ? 1.0f : 0.0f;

					switch (light.Type)
					{
					case Component::Light::EType::Directional:
						lightType = 0.0f;
						data.LightShadowData.y = light.ConstantBias;
						data.LightShadowData.z = light.SlopeBias;
						data.LightShadowData.w = light.NormalOffsetScale;
						break;
					case Component::Light::EType::Point:
						lightType = 1.0f;
						data.Metadata.y = attenuation.constant;
						data.Metadata.z = attenuation.linear;
						data.Metadata.w = attenuation.quadratic;

						lightRange = light.Distance;

						data.LightShadowData.y = light.ConstantBias;
						data.LightShadowData.z = light.SlopeBias;
						data.LightShadowData.w = light.NormalOffsetScale;

						break;
					case Component::Light::EType::Spot:
						lightType = 2.0f;
						data.Metadata.y = glm::cos(glm::radians(light.InnerCutOff));
						data.Metadata.z = glm::cos(glm::radians(light.OuterCutOff));
						data.Metadata.w = 0.0f; // TODO: Calculate spot light angle.

						lightRange = light.Distance;

						data.LightShadowData.y = light.ConstantBias;
						data.LightShadowData.z = light.SlopeBias;
						data.LightShadowData.w = light.NormalOffsetScale;
						break;
					}

					// Calculate light forward direction.
					glm::vec3 rotationRadians = glm::radians(transform.Rotation);
					glm::mat4 localRotation = glm::eulerAngleYXZ(rotationRadians.y, rotationRadians.x, rotationRadians.z);
					glm::vec3 localForward = glm::vec3(0.0f, -1.0f, 0.0f);
					glm::vec3 worldDirection = glm::normalize(glm::mat3(transform.WorldMatrix) * glm::vec3(localRotation * glm::vec4(localForward, 0.0f)));

					glm::vec3 lightDir = worldDirection;
					glm::vec3 lightPos = glm::vec3(transform.WorldMatrix[3]);
					glm::vec3 lightUp = glm::abs(lightDir.y) > 0.99f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);

					glm::mat4 lightView, lightProjection;

					if (light.Type == Component::Light::EType::Directional)
					{
						// Half-size of the area covered by the shadow map.
						const float extent = light.FieldOfView;

						// How far back along -lightDir the eye sits.
						const float distance = 100.0f;

						glm::vec3 focus;

						if (light.FollowMainCamera)
						{
							// Center the shadow volume on the camera so shadows follow the player.
							focus = glm::vec3(sceneRenderData->m_FrameData.ViewPosition);
						}
						else
						{
							focus = lightPos;
						}

						glm::vec3 eye = focus - lightDir * distance;

						lightView = glm::lookAt(eye, focus, lightUp);
						lightProjection = glm::orthoRH_ZO(-extent, extent, -extent, extent, 0.1f, 1000.0f);
					}
					else
					{
						lightView = glm::lookAt(lightPos, lightPos + lightDir, lightUp);
						lightProjection = glm::perspectiveRH_ZO(light.FieldOfView, 1.0f, 0.1f, 1000.0f);
					}

					data.Position = glm::vec4(lightPos, lightType);
					data.Direction = glm::vec4(worldDirection, 0.0f);
					data.Metadata.x = light.Intensity;
					data.Color = glm::vec4(light.Color, lightRange);
					data.LightSpaceMatrix = lightProjection * lightView;

					lightIndex++;
				}
			});

		sceneRenderData->m_FrameData.LightCount = lightIndex;
	}

	// Pass rendering.
	void ForwardPlusSceneRenderer::ShadowPass(CommandBuffer* commandBuffer, ForwardPlusSceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		uint32_t index = 0;

		m_Scene->Filter<Component::Light, Component::Transform>()
			.ForEach([&](Component::Light& light, Component::Transform& transform)
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

							sceneRenderData->m_LightData[index].TileUVRange = tile.GetUVRange();

							uint32_t tileX = tile.x * g_TileSize;
							uint32_t tileY = tile.y * g_TileSize;

							RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_ShadowRenderPass, { tileX, tileY, g_TileSize, g_TileSize });

							Handle<BindGroup> globalShadowBindings = GetShadowBindings();
							ResourceManager::Instance->SetBufferData(globalShadowBindings, 0, (void*)&sceneRenderData->m_LightData[index].LightSpaceMatrix);
							GlobalDrawStream globalDrawStream = { .BindGroup = globalShadowBindings, .UsesDynamicOffset = true, };
							passRenderer->DrawSubPass(globalDrawStream, sceneRenderData->m_ShadowPassStaticMeshDraws);

							commandBuffer->EndRenderPass(*passRenderer);
						}

						index++;
					}
				});

		Renderer::Instance->ShadowAtlasAllocator.Clear();

		ResourceManager::Instance->TransitionTextureLayout(
			commandBuffer,
			Renderer::Instance->ShadowAtlasTexture,
			TextureLayout::DEPTH_STENCIL_ATTACHMENT,
			TextureLayout::DEPTH_STENCIL_READ_ONLY
		);

		END_PROFILE_PASS(Renderer::Instance->GetStats().ShadowPassTime);
	}

	void ForwardPlusSceneRenderer::DepthPrePass(CommandBuffer* commandBuffer, ForwardPlusSceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_DepthOnlyRenderPass);

		Handle<BindGroup> globalBindings = GetGlobalBindings2D();

		// Depth only pre pass for opaque static meshes.
		{
			ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&sceneRenderData->m_FrameData.ViewProjection);
			GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings, .UsesDynamicOffset = true };
			passRenderer->DrawSubPass(globalDrawStream, sceneRenderData->m_PrePassStaticMeshDraws);
		}

		// Depth only pre pass for opaque sprites.
		{
			ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&sceneRenderData->m_FrameData.ViewProjection);
			GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings, .UsesDynamicOffset = true };
			passRenderer->DrawSubPass(globalDrawStream, sceneRenderData->m_PrePassSpriteDraws);
		}

		commandBuffer->EndRenderPass(*passRenderer);

		END_PROFILE_PASS(Renderer::Instance->GetStats().PrePassTime);
	}

	void ForwardPlusSceneRenderer::GridFrustumsComputePass(CommandBuffer* commandBuffer, ForwardPlusSceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		if (!m_GridFrustumsBindGroup.IsValid())
		{
			uint64_t computeVariantHandle = ResourceManager::Instance->GetOrAddShaderVariant(m_GridFrustumsComputeShader, m_GridFrustumsComputeVariant);

			const auto& extents = Window::Instance->GetExtents();
			const uint32_t blockSize = 16;

			CreateGridFrustumsComputeBindGroup(blockSize, extents.x, extents.y, sceneRenderData->m_CameraProjection);
			
			Dispatch dispatch =
			{
				.Shader = m_GridFrustumsComputeShader,
				.BindGroup = m_GridFrustumsBindGroup,
				.ThreadGroupCount = { glm::ceil((extents.x / blockSize) / blockSize), glm::ceil((extents.y / blockSize) / blockSize), 1 },
				.VariantHandle = computeVariantHandle,
			};

			ComputePassRenderer* computePassRenderer = commandBuffer->BeginComputePass({}, { m_OutFrustumsBuffer });
			computePassRenderer->Dispatch({ dispatch });
			commandBuffer->EndComputePass(*computePassRenderer);
		}

		END_PROFILE_PASS(Renderer::Instance->GetStats().SkyboxPassTime);
	}

	void ForwardPlusSceneRenderer::LightCullingComputePass(CommandBuffer* commandBuffer, ForwardPlusSceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		const auto& extents = Window::Instance->GetExtents();
		const uint32_t blockSize = 16;

		if (!m_LightCullingBindGroup.IsValid())
		{
			CreateLightCullingComputeBindGroup(blockSize, extents.x, extents.y, sceneRenderData->m_CameraProjection);
		}

		uint64_t computeVariantHandle = ResourceManager::Instance->GetOrAddShaderVariant(m_GridFrustumsComputeShader, m_GridFrustumsComputeVariant);

		CreateGridFrustumsComputeBindGroup(blockSize, extents.x, extents.y, sceneRenderData->m_CameraProjection);

		Dispatch dispatch =
		{
			.Shader = m_LightCullingComputeShader,
			.BindGroup = m_LightCullingBindGroup,
			.ThreadGroupCount = { glm::ceil((extents.x / blockSize) / blockSize), glm::ceil((extents.y / blockSize) / blockSize), 1 },
			.VariantHandle = computeVariantHandle,
		};

		ComputePassRenderer* computePassRenderer = commandBuffer->BeginComputePass({ /* TODO */ }, { /* TODO */ });
		computePassRenderer->Dispatch({ dispatch });
		commandBuffer->EndComputePass(*computePassRenderer);

		END_PROFILE_PASS(Renderer::Instance->GetStats().SkyboxComputePassTime);
	}

	void ForwardPlusSceneRenderer::PostProcessPass(CommandBuffer* commandBuffer, ForwardPlusSceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		// Transition the layout of the texture that the scene is rendered to, in order to be sampled in the shader.
		ResourceManager::Instance->TransitionTextureLayout(
			commandBuffer,
			Renderer::Instance->IntermediateColorTexture,
			TextureLayout::RENDER_ATTACHMENT,
			TextureLayout::SHADER_READ_ONLY
		);

		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_PostProcessRenderPass);

		ScratchArena scratch(Allocator::FrameArenaRT);
		DrawList draws(scratch, 1);

		draws.Insert({
			.Shader = m_PostProcessShader,
			.VariantHandle = ResourceManager::Instance->GetOrAddShaderVariant(m_PostProcessShader, m_PostProcessShaderVariantHash),
			.VertexBuffer = m_PostProcessQuadVertexBuffer,
			.VertexCount = 6,
		});

		ResourceManager::Instance->SetBufferData(m_PostProcessBindGroup, 0, (void*)&sceneRenderData->m_CameraSettings);
		GlobalDrawStream globalDrawStream = { .BindGroup = m_PostProcessBindGroup };
		passRenderer->DrawSubPass(globalDrawStream, draws);

		commandBuffer->EndRenderPass(*passRenderer);

		END_PROFILE_PASS(Renderer::Instance->GetStats().PostProcessPassTime);
	}

	void ForwardPlusSceneRenderer::PresentPass(CommandBuffer* commandBuffer, ForwardPlusSceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		// Transition the layout of the texture that the scene is rendered to, in order to be sampled in the shader.
		ResourceManager::Instance->TransitionTextureLayout(
			commandBuffer,
			Renderer::Instance->MainColorTexture,
			TextureLayout::RENDER_ATTACHMENT,
			TextureLayout::SHADER_READ_ONLY
		);

		Material* mat = ResourceManager::Instance->GetMaterial(m_QuadMaterial);

		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(Renderer::Instance->GetMainRenderPass());

		ScratchArena scratch(Allocator::FrameArenaRT);
		DrawList draws(scratch, 1);

		draws.Insert({
			.Shader = m_PresentShader,
			.VariantHandle = ResourceManager::Instance->GetOrAddShaderVariant(m_PresentShader, mat->VariantHash),
			.VertexBuffer = m_QuadVertexBuffer,
			.VertexCount = 6,
		});

		GlobalDrawStream globalDrawStream = { .BindGroup = Renderer::Instance->GetGlobalPresentBindings() };
		passRenderer->DrawSubPass(globalDrawStream, draws);

		commandBuffer->EndRenderPass(*passRenderer);

		END_PROFILE_PASS(Renderer::Instance->GetStats().PresentPassTime);
	}

	// Helper.
	void ForwardPlusSceneRenderer::GetViewProjection(ForwardPlusSceneRenderData* sceneRenderData, Entity mainCamera)
	{
		Scene* scene = (Context::Mode == Mode::Editor ? m_EditorScene : m_Scene);

		m_MainCamera = mainCamera;

		if (scene == nullptr || mainCamera == Entity::Null)
		{
			// sceneRenderData->m_OnlyRotationInViewProjection = glm::mat4(1.0f);
			sceneRenderData->m_FrameData.ViewProjection = glm::mat4(1.0f);
			sceneRenderData->m_FrameData.ViewPosition = glm::vec4(0.0f);
			sceneRenderData->m_CameraProjection = glm::mat4(1.0f);
			sceneRenderData->m_CameraSettings.Exposure = 1.0f;
			sceneRenderData->m_CameraSettings.Gamma = 2.2f;
			// sceneRenderData->m_CameraFrustum = {};

			return;
		}

		Component::Camera& camera = scene->GetComponent<Component::Camera>(mainCamera);
		sceneRenderData->m_CameraSettings.Exposure = camera.Exposure;
		sceneRenderData->m_CameraSettings.Gamma = camera.Gamma;
		sceneRenderData->m_FrameData.ViewProjection = camera.ViewProjectionMatrix;
		// sceneRenderData->m_CameraFrustum = camera.Frustum;

		Component::Transform& tr = scene->GetComponent<Component::Transform>(mainCamera);
		sceneRenderData->m_FrameData.ViewPosition = tr.WorldMatrix[3];
		// sceneRenderData->m_OnlyRotationInViewProjection = camera.Projection * glm::mat4(glm::mat3(camera.View));
		sceneRenderData->m_CameraProjection = camera.Projection;
	}

	void ForwardPlusSceneRenderer::CreateGridFrustumsComputeBindGroup(uint32_t blockSize, uint32_t width, uint32_t height, const glm::mat4& cameraProjection)
	{
		// Create DispatchParams uniform buffer.
		DispatchParams dispatchParams =
		{
			.numThreadGroups = { glm::ceil((height / blockSize) / blockSize), glm::ceil((height / blockSize) / blockSize), 1 },
			.numThreads = { height / blockSize, height / blockSize, 1 },
		};

		Handle<Buffer> dispatchParamsBuffer = ResourceManager::Instance->CreateBuffer({
			.debugName = "dispatch-params-buffer",
			.usage = BufferUsage::UNIFORM,
			.memoryUsage = MemoryUsage::CPU_GPU,
			.byteSize = sizeof(DispatchParams),
			.initialData = &dispatchParams,
		});

		// Create ScreenToViewParams uniform buffer.
		ScreenToViewParams screenToViewParams =
		{
			.InverseProjection = glm::inverse(cameraProjection),
			.ScreenDimensions = { width, height }
		};

		Handle<Buffer> screenToViewParamsBuffer = ResourceManager::Instance->CreateBuffer({
			.debugName = "screen-to-view-params-buffer",
			.usage = BufferUsage::UNIFORM,
			.memoryUsage = MemoryUsage::CPU_GPU,
			.byteSize = sizeof(ScreenToViewParams),
			.initialData = &screenToViewParams,
		});

		// Create Frustum storage buffer.
		const uint32_t numFrustums = (width / blockSize) * (height / blockSize);

		m_OutFrustumsBuffer = ResourceManager::Instance->CreateBuffer({
			.debugName = "out-frustums-buffer",
			.usage = BufferUsage::STORAGE,
			.memoryUsage = MemoryUsage::GPU_CPU,
			.byteSize = (uint32_t)sizeof(Frustum) * numFrustums,
		});

		// Create grid frustums bindGroup.
		m_GridFrustumsBindGroup = ResourceManager::Instance->CreateBindGroup({
			.debugName = "grid-frustums-bind-group",
			.layout = m_GridFrustumsBindGroupLayout,
			.buffers = {
				{ dispatchParamsBuffer },
				{ screenToViewParamsBuffer },
				{ m_OutFrustumsBuffer },
			}
		});
	}

	void ForwardPlusSceneRenderer::CreateLightCullingComputeBindGroup(uint32_t blockSize, uint32_t width, uint32_t height, const glm::mat4& cameraProjection)
	{

	}
}
