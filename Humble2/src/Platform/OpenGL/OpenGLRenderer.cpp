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
		default:
			assert(false);
			return nullptr;
		}
	}

	void OpenGLRenderer::EndFrame()
	{
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
