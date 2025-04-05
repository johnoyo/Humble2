#include "OpenGLRenderer.h"

namespace HBL2
{
	void OpenGLRenderer::PreInitialize()
	{
		m_GraphicsAPI = GraphicsAPI::OPENGL;
		m_ResourceManager = (OpenGLResourceManager*)ResourceManager::Instance;

#ifdef DEBUG
		GLDebug::EnableGLDebugging();
#endif
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);

		// m_ShadowCommandBuffer = new OpenGLCommandBuffer();
		m_PrePassCommandBuffer = new OpenGLCommandBuffer();
		m_OpaqueCommandBuffer = new OpenGLCommandBuffer();
		// m_SkyboxCommandBuffer = new OpenGLCommandBuffer();
		m_TransparentCommandBuffer = new OpenGLCommandBuffer();
		// m_PostProcessCommandBuffer = new OpenGLCommandBuffer();
		m_PresentCommandBuffer = new OpenGLCommandBuffer();
		m_UserInterfaceCommandBuffer = new OpenGLCommandBuffer();

		CreateBindings();
		CreateRenderPass();
	}

	void OpenGLRenderer::PostInitialize()
	{
		m_GlobalPresentBindings = m_ResourceManager->CreateBindGroup({
			.debugName = "global-present-bind-group",
			.layout = Renderer::Instance->GetGlobalPresentBindingsLayout(),
			.textures = { Renderer::Instance->MainColorTexture },
		});

		EventDispatcher::Get().Register<FramebufferSizeEvent>([&](const FramebufferSizeEvent& e)
		{
			m_Resize = true;
			m_NewSize = { e.Width, e.Height };
		});
	}

	void OpenGLRenderer::BeginFrame()
	{
		m_Stats.Reset();
		TempUniformRingBuffer->Invalidate();
	}

	CommandBuffer* OpenGLRenderer::BeginCommandRecording(CommandBufferType type, RenderPassStage stage)
	{
		switch (type)
		{
		case HBL2::CommandBufferType::MAIN:
			switch (stage)
			{
			case HBL2::RenderPassStage::Shadow:
				return m_ShadowCommandBuffer;
			case HBL2::RenderPassStage::PrePass:
				return m_PrePassCommandBuffer;
			case HBL2::RenderPassStage::Opaque:
				return m_OpaqueCommandBuffer;
			case HBL2::RenderPassStage::Skybox:
				return m_SkyboxCommandBuffer;
			case HBL2::RenderPassStage::Transparent:
				return m_TransparentCommandBuffer;
			case HBL2::RenderPassStage::PostProcess:
				return m_PostProcessCommandBuffer;
			case HBL2::RenderPassStage::Present:
				return m_PresentCommandBuffer;
			}
			break;
		case HBL2::CommandBufferType::CUSTOM:
			HBL2_CORE_ASSERT(false, "Custom Command buffers not implemented yet!");
			break;
		case HBL2::CommandBufferType::UI:
			return m_UserInterfaceCommandBuffer;
		}

		return nullptr;
	}

	void OpenGLRenderer::EndFrame()
	{
		m_ResourceManager->Flush(m_FrameNumber);
	}

	void OpenGLRenderer::Present()
	{
		glfwSwapBuffers(Window::Instance->GetHandle());
		m_FrameNumber++;

		if (m_Resize)
		{
			Resize(m_NewSize.x, m_NewSize.y);
			m_Resize = false;
		}
	}

	void OpenGLRenderer::Resize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
		{
			return;
		}

		// Destroy old offscreen textures
		m_ResourceManager->DeleteTexture(MainColorTexture);
		m_ResourceManager->DeleteTexture(MainDepthTexture);

		// Recreate the offscreen texture (render target)
		MainColorTexture = ResourceManager::Instance->CreateTexture({
			.debugName = "viewport-color-target",
			.dimensions = { width, height, 1 },
			.format = Format::BGRA8_UNORM,
			.internalFormat = Format::BGRA8_UNORM,
			.usage = { TextureUsage::RENDER_ATTACHMENT, TextureUsage::SAMPLED },
			.aspect = TextureAspect::COLOR,
			.sampler =
			{
				.filter = Filter::LINEAR,
				.wrap = Wrap::CLAMP_TO_EDGE,
			}
		});

		MainDepthTexture = ResourceManager::Instance->CreateTexture({
			.debugName = "viewport-depth-target",
			.dimensions = { width, height, 1 },
			.format = Format::D32_FLOAT,
			.internalFormat = Format::D32_FLOAT,
			.usage = TextureUsage::DEPTH_STENCIL,
			.aspect = TextureAspect::DEPTH,
			.createSampler = true,
			.sampler =
			{
				.filter = Filter::NEAREST,
				.wrap = Wrap::CLAMP_TO_EDGE,
			}
		});

		// Update descriptor sets (for the quad shader)
		m_ResourceManager->DeleteBindGroup(m_GlobalPresentBindings);

		m_GlobalPresentBindings = m_ResourceManager->CreateBindGroup({
			.debugName = "global-present-bind-group",
			.layout = Renderer::Instance->GetGlobalPresentBindingsLayout(),
			.textures = { MainColorTexture },  // Updated with new size
		});

		OpenGLBindGroup* globalPresentBindings = m_ResourceManager->GetBindGroup(m_GlobalPresentBindings);
		globalPresentBindings->Set();

		// Call on resize callbacks.
		for (auto&& [name, callback] : m_OnResizeCallbacks)
		{
			callback(width, height);
		}
	}

	void OpenGLRenderer::Clean()
	{
		m_ResourceManager->DeleteTexture(MainColorTexture);
		m_ResourceManager->DeleteTexture(MainDepthTexture);

		TempUniformRingBuffer->Free();

		m_ResourceManager->DeleteBindGroupLayout(m_GlobalBindingsLayout2D);
		m_ResourceManager->DeleteBindGroupLayout(m_GlobalBindingsLayout3D);
		m_ResourceManager->DeleteBindGroupLayout(m_GlobalPresentBindingsLayout);

		m_ResourceManager->DeleteBindGroup(m_GlobalBindings2D);
		m_ResourceManager->DeleteBindGroup(m_GlobalBindings3D);
		m_ResourceManager->DeleteBindGroup(m_GlobalPresentBindings);

		m_ResourceManager->DeleteRenderPass(m_RenderPass);
		m_ResourceManager->DeleteFrameBuffer(m_MainFrameBuffer);
	}

	void* OpenGLRenderer::GetDepthAttachment()
	{
		if (MainDepthTexture.IsValid())
		{
			OpenGLTexture* openGLTexture = m_ResourceManager->GetTexture(MainDepthTexture);
			return (void*)(intptr_t)openGLTexture->RendererId;
		}

		return nullptr;
	}

	void* OpenGLRenderer::GetColorAttachment()
	{
		if (MainColorTexture.IsValid())
		{
			OpenGLTexture* openGLTexture = m_ResourceManager->GetTexture(MainColorTexture);
			return (void*)(intptr_t)openGLTexture->RendererId;
		}

		return nullptr;
	}

	void OpenGLRenderer::CreateBindings()
	{
		// Global bindings for the 2D rendering.
		m_GlobalBindingsLayout2D = m_ResourceManager->CreateBindGroupLayout({
			.debugName = "unlit-colored-layout",
			.bufferBindings = {
				{
					.slot = 0,
					.visibility = ShaderStage::VERTEX,
					.type = BufferBindingType::UNIFORM,
				},
			},
		});

		auto cameraBuffer2D = m_ResourceManager->CreateBuffer({
			.debugName = "camera-uniform-buffer",
			.usage = BufferUsage::UNIFORM,
			.usageHint = BufferUsageHint::DYNAMIC,
			.memoryUsage = MemoryUsage::GPU_CPU,
			.byteSize = 64,
			.initialData = nullptr
		});

		m_GlobalBindings2D = m_ResourceManager->CreateBindGroup({
			.debugName = "unlit-colored-bind-group",
			.layout = m_GlobalBindingsLayout2D,
			.buffers = {
				{ .buffer = cameraBuffer2D },
			}
		});

		// Global bindings for the 3D rendering.
		m_GlobalBindingsLayout3D = m_ResourceManager->CreateBindGroupLayout({
			.debugName = "global-bind-group-layout",
			.bufferBindings = {
				{
					.slot = 0,
					.visibility = ShaderStage::VERTEX,
					.type = BufferBindingType::UNIFORM,
				},
				{
					.slot = 1,
					.visibility = ShaderStage::FRAGMENT,
					.type = BufferBindingType::UNIFORM,
				},
			},
		});

		auto cameraBuffer3D = m_ResourceManager->CreateBuffer({
			.debugName = "camera-uniform-buffer",
			.usage = BufferUsage::UNIFORM,
			.usageHint = BufferUsageHint::DYNAMIC,
			.memoryUsage = MemoryUsage::GPU_CPU,
			.byteSize = sizeof(CameraData),
			.initialData = nullptr
		});

		auto lightBuffer = m_ResourceManager->CreateBuffer({
			.debugName = "light-uniform-buffer",
			.usage = BufferUsage::UNIFORM,
			.usageHint = BufferUsageHint::DYNAMIC,
			.memoryUsage = MemoryUsage::GPU_CPU,
			.byteSize = sizeof(LightData),
			.initialData = nullptr
		});

		m_GlobalBindings3D = m_ResourceManager->CreateBindGroup({
			.debugName = "global-bind-group",
			.layout = m_GlobalBindingsLayout3D,
			.buffers = {
				{ .buffer = cameraBuffer3D },
				{ .buffer = lightBuffer },
			}
		});

		// Global bindings for presenting to offscreen texture
		m_GlobalPresentBindingsLayout = m_ResourceManager->CreateBindGroupLayout({
			.debugName = "global-present-bind-group-layout",
			.textureBindings = {
				{
					.slot = 0,
					.visibility = ShaderStage::FRAGMENT,
				},
			},
		});
	}

	void OpenGLRenderer::CreateRenderPass()
	{
		Handle<RenderPassLayout> renderPassLayout = ResourceManager::Instance->CreateRenderPassLayout({
			.debugName = "main-renderpass-layout",
			.depthTargetFormat = Format::D32_FLOAT,
			.subPasses = {
				{ .depthTarget = true, .colorTargets = 1, },
			},
		});

		m_RenderPass = ResourceManager::Instance->CreateRenderPass({
			.debugName = "main-renderpass",
			.layout = renderPassLayout,
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
	}
}
