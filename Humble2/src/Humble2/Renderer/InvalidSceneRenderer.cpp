#include "InvalidSceneRenderer.h"

#include "Core/Window.h"
#include "Core/Context.h"
#include "Utilities/ShaderUtilities.h"

namespace HBL2
{
	using packed_size = ShaderDescriptor::RenderPipeline::packed_size;

	void InvalidSceneRenderer::Initialize(Scene* scene, uint32_t maxLights, bool useEditorCamera)
	{
		m_Scene = scene;

		m_ResourceManager = ResourceManager::Instance;
		m_EditorScene = m_ResourceManager->GetScene(Context::EditorScene);
		m_UniformRingBuffer = Renderer::Instance->TempUniformRingBuffer;

		TextureSetup();
		RenderPassSetup();

		// Setup render passes.
		PinkQuadPassSetup();

		m_Initialized = true;
	}

	void InvalidSceneRenderer::Gather(Entity mainCamera)
	{
		InvalidSceneRenderData* sceneRenderData = &m_RenderData[Renderer::Instance->GetFrameWriteIndex()];
	}

	void InvalidSceneRenderer::Render(CommandBuffer* commandBuffer, uint32_t renderDataIndex)
	{
		BEGIN_PROFILE_PASS();

		InvalidSceneRenderData* sceneRenderData = (InvalidSceneRenderData*)&m_RenderData[renderDataIndex];
		ResourceManager* rm = ResourceManager::Instance;

		rm->TransitionTextureLayout(commandBuffer, m_IntermediateTexture, TextureLayout::UNDEFINED, TextureLayout::RENDER_ATTACHMENT);
		rm->TransitionTextureLayout(commandBuffer, m_TargetTexture, TextureLayout::UNDEFINED, TextureLayout::RENDER_ATTACHMENT);
		rm->TransitionTextureLayout(commandBuffer, m_DepthTexture, TextureLayout::UNDEFINED, TextureLayout::DEPTH_STENCIL_READ_ONLY);

		PinkQuadPass(commandBuffer, sceneRenderData);

		END_PROFILE_PASS(Renderer::Instance->GetStats().MainPassTime);
	}

	void InvalidSceneRenderer::CleanUp()
	{
		m_ResourceManager->DeleteRenderPassLayout(m_RenderPassLayout);
		m_ResourceManager->DeleteTexture(m_TargetTexture);
		m_ResourceManager->DeleteTexture(m_IntermediateTexture);
		m_ResourceManager->DeleteTexture(m_DepthTexture);
		EventDispatcher::Get().Deregister<RendererResizedEvent>(m_RendererResizeEventHandle);

		// Pink quad pass clean up.
		m_ResourceManager->DeleteBuffer(m_PinkQuadBuffer);
		m_ResourceManager->DeleteShader(m_PinkQuadShader);
		m_ResourceManager->DeleteRenderPass(m_PinkQuadRenderPass);
		EventDispatcher::Get().Deregister<RendererResizedEvent>(m_PinkQuadPassResizeEventHandle);
		m_ResourceManager->DeleteBuffer(m_PinkQuadVertexBuffer);

		m_Initialized = false;
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
				.depthTarget = m_DepthTexture,
				.colorTargets = { m_TargetTexture },
			}
		});

		m_PinkQuadPassResizeEventHandle = EventDispatcher::Get().Register<RendererResizedEvent>([this](const RendererResizedEvent& event)
		{
			ResourceManager::Instance->RecreateRenderPassFrameBuffer(m_PinkQuadRenderPass, {
				.width = event.Width,
				.height = event.Height,
				.depthTarget = m_DepthTexture,
				.colorTargets = { m_TargetTexture },
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

	void InvalidSceneRenderer::TextureSetup()
	{
		CreateTextures();
	}

	// Pass rendering.
	void InvalidSceneRenderer::PinkQuadPass(CommandBuffer* commandBuffer, InvalidSceneRenderData* sceneRenderData)
	{
		BEGIN_PROFILE_PASS();

		// Transition the layout of the texture that the scene is rendered to, in order to be sampled in the shader.
		ResourceManager::Instance->TransitionTextureLayout(
			commandBuffer,
			m_IntermediateTexture,
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
}
