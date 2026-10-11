#include "InvalidSceneRenderer.h"

#include "Core/Window.h"
#include "Core/Context.h"
#include "Utilities/ShaderUtilities.h"

namespace HBL2
{
	using packed_size = ShaderDescriptor::RenderPipeline::packed_size;

	void InvalidSceneRenderer::Initialize(Scene* scene, uint32_t maxLights)
	{
		m_Scene = scene;

		m_ResourceManager = ResourceManager::Instance;
		m_EditorScene = m_ResourceManager->GetScene(Context::EditorScene);
		m_UniformRingBuffer = Renderer::Instance->TempUniformRingBuffer;

		RenderPassSetup();

		// Setup render passes.
		PinkQuadPassSetup();
		PresentPassSetup();
	}

	void InvalidSceneRenderer::Gather(Entity mainCamera)
	{
		InvalidSceneRenderData* sceneRenderData = &m_RenderData[Renderer::Instance->GetFrameWriteIndex()];
	}

	void InvalidSceneRenderer::Render(void* renderData, void* debugRenderData)
	{
		BEGIN_PROFILE_PASS();

		InvalidSceneRenderData* sceneRenderData = (InvalidSceneRenderData*)renderData;
		ResourceManager* rm = ResourceManager::Instance;

		CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::MAIN);

		rm->TransitionTextureLayout(commandBuffer, Renderer::Instance->IntermediateColorTexture, TextureLayout::UNDEFINED, TextureLayout::RENDER_ATTACHMENT);
		rm->TransitionTextureLayout(commandBuffer, Renderer::Instance->MainColorTexture, TextureLayout::UNDEFINED, TextureLayout::RENDER_ATTACHMENT);
		rm->TransitionTextureLayout(commandBuffer, Renderer::Instance->MainDepthTexture, TextureLayout::UNDEFINED, TextureLayout::DEPTH_STENCIL_READ_ONLY);

		PinkQuadPass(commandBuffer, sceneRenderData);
		PresentPass(commandBuffer, sceneRenderData);

		commandBuffer->EndCommandRecording();
		commandBuffer->Submit();

		END_PROFILE_PASS(Renderer::Instance->GetStats().MainPassTime);
	}

	void InvalidSceneRenderer::CleanUp()
	{
		m_ResourceManager->DeleteRenderPassLayout(m_RenderPassLayout);

		// Post process pass clean up.
		m_ResourceManager->DeleteBuffer(m_PinkQuadBuffer);
		m_ResourceManager->DeleteShader(m_PinkQuadShader);
		m_ResourceManager->DeleteRenderPass(m_PinkQuadRenderPass);
		Renderer::Instance->RemoveOnResizeCallback(std::string("Invalid-Renderer-Resize-FrameBuffer-") + m_Scene->GetDescriptor().name.c_str());
		m_ResourceManager->DeleteBuffer(m_PinkQuadVertexBuffer);

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
	}

	void* InvalidSceneRenderer::GetRenderData()
	{
		return &m_RenderData[Renderer::Instance->GetFrameWriteIndex()];
	}

	// Scene renderer set up.
	void InvalidSceneRenderer::RenderPassSetup()
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

	// Pass set up.
	void InvalidSceneRenderer::PinkQuadPassSetup()
	{
		float* vertexBuffer = new float[12] {
			-1.0, -1.0, // Bottom left
			 1.0, -1.0, // Bottom right
			 1.0,  1.0, // Top right
			 1.0,  1.0, // Top right
			-1.0,  1.0, // Top left
			-1.0, -1.0, // Bottom left
		};

		m_PinkQuadVertexBuffer = m_ResourceManager->CreateBuffer({
			.debugName = "quad-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.byteSize = sizeof(float) * 12,
			.initialData = vertexBuffer,
		});

		// Create renderpass and framebuffer.
		m_PinkQuadRenderPass = m_ResourceManager->CreateRenderPass({
			.debugName = "pink-quad-renderpass",
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

		Renderer::Instance->AddCallbackOnResize(std::string("Invalid-Renderer-Resize-FrameBuffer-") + m_Scene->GetDescriptor().name.c_str(), [this](uint32_t width, uint32_t height)
		{
			ResourceManager::Instance->RecreateRenderPassFrameBuffer(m_PinkQuadRenderPass, {
				.width = width,
				.height = height,
				.depthTarget = Renderer::Instance->MainDepthTexture,
				.colorTargets = { Renderer::Instance->MainColorTexture },
			});
		});

		// Create invalid-renderer shader.
		const auto& invalidShaderData = ShaderUtilities::Get().Compile("assets/shaders/invalid-renderer.slang", (ShaderReflectionData*)nullptr, false);

		ShaderDescriptor::RenderPipeline::PackedVariant variant = {};
		variant.blendEnabled = false;
		variant.depthWrite = false;
		variant.frontFace = (packed_size)FrontFace::COUNTER_CLOCKWISE;

		m_PinkQuadShader = ResourceManager::Instance->CreateShader({
			.debugName = "invalid-renderer-shader",
			.VS { .code = invalidShaderData.vertexShaderCode.AsSpan(), .entryPoint = "mainVS" },
			.FS { .code = invalidShaderData.fragmentShaderCode.AsSpan(), .entryPoint = "mainPS" },
			.bindGroups { },
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = 8,
						.attributes = {
							{ .byteOffset = 0, .format = VertexFormat::FLOAT32x2 },
						},
					}
				},
				.variants = { variant },
			},
			.renderPass = m_PinkQuadRenderPass,
		});

		// Cache post-process variant hash.
		m_PinkQuadShaderVariantHash = variant;
	}

	void InvalidSceneRenderer::PresentPassSetup()
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
		variant.frontFace = (packed_size)FrontFace::COUNTER_CLOCKWISE;

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
							{.byteOffset = 0, .format = VertexFormat::FLOAT32x2 },
							{.byteOffset = 8, .format = VertexFormat::FLOAT32x2 },
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
	void InvalidSceneRenderer::PinkQuadPass(CommandBuffer* commandBuffer, InvalidSceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		// Transition the layout of the texture that the scene is rendered to, in order to be sampled in the shader.
		ResourceManager::Instance->TransitionTextureLayout(
			commandBuffer,
			Renderer::Instance->IntermediateColorTexture,
			TextureLayout::RENDER_ATTACHMENT,
			TextureLayout::SHADER_READ_ONLY
		);

		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_PinkQuadRenderPass);

		ScratchArena scratch(Allocator::FrameArenaRT);
		DrawList draws(scratch, 1);

		draws.Insert({
			.Shader = m_PinkQuadShader,
			.VariantHandle = ResourceManager::Instance->GetOrAddShaderVariant(m_PinkQuadShader, m_PinkQuadShaderVariantHash),
			.VertexBuffer = m_PinkQuadVertexBuffer,
			.VertexCount = 6,
		});

		GlobalDrawStream globalDrawStream = { };
		passRenderer->DrawSubPass(globalDrawStream, draws);

		commandBuffer->EndRenderPass(*passRenderer);

		END_PROFILE_PASS(Renderer::Instance->GetStats().PostProcessPassTime);
	}

	void InvalidSceneRenderer::PresentPass(CommandBuffer* commandBuffer, InvalidSceneRenderData* sceneRenderData)
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
}
