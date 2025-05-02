#include "OpenGLCommandBuffer.h"

#include "OpenGLRenderer.h"
#include "OpenGLResourceManager.h"

namespace HBL2
{
    RenderPassRenderer* OpenGLCommandBuffer::BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer)
    {
        OpenGLResourceManager* rm = (OpenGLResourceManager*)ResourceManager::Instance;
        OpenGLRenderer* renderer = (OpenGLRenderer*)Renderer::Instance;

		// Bind framebuffer and set viewport
		m_FrameBuffer = frameBuffer;

		if (frameBuffer.IsValid())
		{
			OpenGLFrameBuffer* openGLFrameBuffer = rm->GetFrameBuffer(frameBuffer);

			glBindFramebuffer(GL_FRAMEBUFFER, openGLFrameBuffer->RendererId);
			glViewport(0, 0, openGLFrameBuffer->Width, openGLFrameBuffer->Height);
		}

		if (!renderPass.IsValid())
		{
			return &m_CurrentRenderPassRenderer;
		}

		// Get clear values from render pass
		OpenGLRenderPass* openGLRenderPass = rm->GetRenderPass(renderPass);		

		if (openGLRenderPass->ColorTargetCount == 0)
		{
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		}
		else
		{
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		}

		GLenum clearValues = 0;
		for (bool clearValue : openGLRenderPass->ColorClearValues)
		{
			if (clearValue)
			{
				clearValues = GL_COLOR_BUFFER_BIT;
			}
		}

		if (openGLRenderPass->DepthClearValue)
		{
			clearValues |= GL_DEPTH_BUFFER_BIT;
		}

		if (openGLRenderPass->StencilClearValue)
		{
			clearValues |= GL_STENCIL_BUFFER_BIT;
		}

		// Clear screen if needed.
		if (clearValues != 0)
		{
			glClearColor(openGLRenderPass->ClearColor.r, openGLRenderPass->ClearColor.g, openGLRenderPass->ClearColor.b, openGLRenderPass->ClearColor.a);
			glClearDepth(openGLRenderPass->ClearDepth);
			glClearStencil(openGLRenderPass->ClearStencil);
			glClear(clearValues);
		}

        return &m_CurrentRenderPassRenderer;
    }

    void OpenGLCommandBuffer::EndRenderPass(const RenderPassRenderer& renderPassRenderer)
    {
		if (m_FrameBuffer.IsValid())
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
    }

	ComputePassRenderer* OpenGLCommandBuffer::BeginComputePass(const Span<const Handle<Texture>>& texturesWrite, const Span<const Handle<Buffer>>& buffersWrite)
	{
		OpenGLResourceManager* rm = (OpenGLResourceManager*)ResourceManager::Instance;
		OpenGLRenderer* renderer = (OpenGLRenderer*)Renderer::Instance;

		uint32_t slot = 0;

		for (const auto& textureHandle : texturesWrite)
		{
			OpenGLTexture* texture = rm->GetTexture(textureHandle);
			texture->Bind(slot++);
		}

		for (const auto& bufferHandle : buffersWrite)
		{
			OpenGLBuffer* buffer = rm->GetBuffer(bufferHandle);
			glBindBufferBase(GL_UNIFORM_BUFFER, slot++, buffer->RendererId);
		}

		return &m_CurrentComputePassRenderer;
	}

	void OpenGLCommandBuffer::EndComputePass(const ComputePassRenderer& computePassRenderer)
	{
		glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
		return;
	}

	void OpenGLCommandBuffer::EndCommandRecording()
	{
		return;
	}

	void OpenGLCommandBuffer::Submit()
    {
		return;
    }
}
