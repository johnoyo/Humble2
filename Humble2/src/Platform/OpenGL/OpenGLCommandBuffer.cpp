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

		// Get clear values from render pass
		OpenGLRenderPass* openGLRenderPass = rm->GetRenderPass(renderPass);		

		GLenum clearValues = 0;
		for (auto clearValue : openGLRenderPass->ColorClearValues)
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

		// Clear screen if needed.
		if (clearValues != 0)
		{
			glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
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

    void OpenGLCommandBuffer::Submit()
    {
    }
}
