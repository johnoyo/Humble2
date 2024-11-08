#include "OpenGLRenderer.h"

namespace HBL2
{
	void OpenGLRenderer::InitializeInternal()
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

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		m_OpaqueCommandBuffer = new OpenGLCommandBuffer();
		m_OpaqueSpriteCommandBuffer = new OpenGLCommandBuffer();
		m_PresentCommandBuffer = new OpenGLCommandBuffer();
		m_UserInterfaceCommandBuffer = new OpenGLCommandBuffer();

		CreateBindings();
		CreateRenderPass();
	}

	void OpenGLRenderer::BeginFrame()
	{
		TempUniformRingBuffer->Invalidate();
	}

	void OpenGLRenderer::SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData)
	{
		OpenGLBuffer* openGLBuffer = m_ResourceManager->GetBuffer(buffer);
		openGLBuffer->Data = newData;
	}

	void OpenGLRenderer::SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData)
	{
		OpenGLBindGroup* openGLBindGroup = m_ResourceManager->GetBindGroup(bindGroup);
		if (bufferIndex < openGLBindGroup->Buffers.size())
		{
			SetBufferData(openGLBindGroup->Buffers[bufferIndex].buffer, openGLBindGroup->Buffers[bufferIndex].byteOffset, newData);
		}
	}

	void OpenGLRenderer::Draw(Handle<Mesh> mesh)
	{
		Mesh* openGLMesh = m_ResourceManager->GetMesh(mesh);
		glDrawArrays(GL_TRIANGLES, openGLMesh->VertexOffset, openGLMesh->VertexCount);
	}

	void OpenGLRenderer::DrawIndexed(Handle<Mesh> mesh)
	{
		Mesh* openGLMesh = m_ResourceManager->GetMesh(mesh);
		glDrawElements(GL_TRIANGLES, (openGLMesh->IndexCount - openGLMesh->IndexOffset), GL_UNSIGNED_INT, nullptr);
	}

	CommandBuffer* OpenGLRenderer::BeginCommandRecording(CommandBufferType type, RenderPassStage stage)
	{
		switch (type)
		{
		case HBL2::CommandBufferType::MAIN:
			switch (stage)
			{
			case HBL2::RenderPassStage::Shadow:
				break;
			case HBL2::RenderPassStage::Opaque:
				return m_OpaqueCommandBuffer;
			case HBL2::RenderPassStage::Skybox:
				break;
			case HBL2::RenderPassStage::Transparent:
				break;
			case HBL2::RenderPassStage::OpaqueSprite:
				return m_OpaqueSpriteCommandBuffer;
			case HBL2::RenderPassStage::PostProcess:
				break;
			case HBL2::RenderPassStage::Present:
				return m_PresentCommandBuffer;
			}
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
	}

	void OpenGLRenderer::Present()
	{
		glfwSwapBuffers(Window::Instance->GetHandle());
	}

	void OpenGLRenderer::Clean()
	{
		// TODO!
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
				{.buffer = cameraBuffer2D },
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
				{.buffer = cameraBuffer3D },
				{.buffer = lightBuffer },
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
