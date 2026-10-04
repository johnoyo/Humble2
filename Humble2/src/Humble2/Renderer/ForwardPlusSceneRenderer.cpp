#include "ForwardPlusSceneRenderer.h"

#include "Core/Window.h"
#include "Core/Context.h"
#include "Utilities/ShaderUtilities.h"

namespace HBL2
{
	using packed_size = ShaderDescriptor::RenderPipeline::packed_size;

	void ForwardPlusSceneRenderer::Initialize(Scene* scene, uint32_t maxLights)
	{
		m_Scene = scene;

		m_ResourceManager = ResourceManager::Instance;
		m_EditorScene = m_ResourceManager->GetScene(Context::EditorScene);
		m_UniformRingBuffer = Renderer::Instance->TempUniformRingBuffer;

		RenderPassSetup();
		BindingsSetup();

		// Setup render passes.
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
		// rm->TransitionTextureLayout(commandBuffer, Renderer::Instance->ShadowAtlasTexture, TextureLayout::UNDEFINED, TextureLayout::DEPTH_STENCIL_ATTACHMENT);		

		rm->TransitionTextureLayout(commandBuffer, Renderer::Instance->MainDepthTexture, TextureLayout::UNDEFINED, TextureLayout::DEPTH_STENCIL_READ_ONLY);

		// Shadow pre-pass
		
		// Depth pre-pass

		// Grid Frustums Compute Shader
		// RWStructuredBuffer<Frustum> out_Frustums : register(u0);

		// Light Culling Compute Shader
		// Texture2D DepthTextureVS : register(t3);
		// StructuredBuffer<Frustum> in_Frustums : register(t9);

		// Geometry pass

		// Post process pass
		PostProcessPass(commandBuffer, sceneRenderData);

		// Debug pass

		// Present pass
		PresentPass(commandBuffer, sceneRenderData);

		commandBuffer->EndCommandRecording();
		commandBuffer->Submit();

		END_PROFILE_PASS(Renderer::Instance->GetStats().MainPassTime);
	}

	void ForwardPlusSceneRenderer::CleanUp()
	{
		m_ResourceManager->DeleteRenderPassLayout(m_RenderPassLayout);

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
	}

	// Gathering.
	void ForwardPlusSceneRenderer::GatherDraws(ForwardPlusSceneRenderData* sceneRenderData)
	{

	}

	// Pass set up.
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

	// Pass rendering.
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

	void ForwardPlusSceneRenderer::GetViewProjection(ForwardPlusSceneRenderData* sceneRenderData, Entity mainCamera)
	{

	}
}
