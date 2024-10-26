#include "OpenGLRenderer.h"

namespace HBL2
{
	void OpenGLRenderer::Initialize()
	{
		m_GraphicsAPI = GraphicsAPI::OPENGL;
		m_ResourceManager = (OpenGLResourceManager*)ResourceManager::Instance;

		TempUniformRingBuffer = new UniformRingBuffer(4096, Device::Instance->GetGPUProperties().limits.minUniformBufferOffsetAlignment);

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

		m_MainCommandBuffer = new OpenGLCommandBuffer();
		m_OffscreenCommandBuffer = new OpenGLCommandBuffer();
		m_UserInterfaceCommandBuffer = new OpenGLCommandBuffer();

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
	}

	void OpenGLRenderer::BeginFrame()
	{
		if (Context::FrameBuffer.IsValid())
		{
			OpenGLFrameBuffer* openGLFrameBuffer = m_ResourceManager->GetFrameBuffer(Context::FrameBuffer);

			glBindFramebuffer(GL_FRAMEBUFFER, openGLFrameBuffer->RendererId);
			glViewport(0, 0, openGLFrameBuffer->Width, openGLFrameBuffer->Height);
		}

		glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

	CommandBuffer* OpenGLRenderer::BeginCommandRecording(CommandBufferType type)
	{
		switch (type)
		{
		case HBL2::CommandBufferType::MAIN:
			return m_MainCommandBuffer;
		case HBL2::CommandBufferType::OFFSCREEN:
			return m_OffscreenCommandBuffer;
		case HBL2::CommandBufferType::UI:
			return m_UserInterfaceCommandBuffer;
		}

		return nullptr;
	}

	void OpenGLRenderer::EndFrame()
	{
		//CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::MAIN);
		//RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(Renderer::Instance->GetMainRenderPass(), Renderer::Instance->GetMainFrameBuffer());

		////GlobalDrawStream globalDrawStream3D = { .BindGroup = GetGlobalBindings3D() };
		////passRenderer->DrawSubPass(globalDrawStream3D, m_DrawList3D);

		//GlobalDrawStream globalDrawStream2D = { .BindGroup = GetGlobalBindings2D() };
		//passRenderer->DrawSubPass(globalDrawStream2D, m_DrawList2D);

		//commandBuffer->EndRenderPass(*passRenderer);
		//commandBuffer->Submit();

		//m_DrawList2D.Reset();
		//m_DrawList3D.Reset();

		if (Context::FrameBuffer.IsValid())
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
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
		if (Context::FrameBuffer.IsValid())
		{
			OpenGLFrameBuffer* openGLFrameBuffer = m_ResourceManager->GetFrameBuffer(Context::FrameBuffer);
			return (void*)(intptr_t)openGLFrameBuffer->DepthAttachmentId;
		}

		return nullptr;
	}

	void* OpenGLRenderer::GetColorAttachment()
	{
		if (Context::FrameBuffer.IsValid())
		{
			OpenGLFrameBuffer* openGLFrameBuffer = m_ResourceManager->GetFrameBuffer(Context::FrameBuffer);
			return (void*)(intptr_t)openGLFrameBuffer->ColorAttachmentId;
		}

		return nullptr;
	}
}
