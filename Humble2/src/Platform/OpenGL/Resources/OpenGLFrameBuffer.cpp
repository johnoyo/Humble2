#include "OpenGLFrameBuffer.h"

#include "Platform\OpenGL\OpenGLResourceManager.h"

namespace HBL2
{
	void OpenGLFrameBuffer::Create()
	{
		if (RendererId)
		{
			Destroy();
		}

		// Build the framebuffer.
		glGenFramebuffers(1, &RendererId);
		glBindFramebuffer(GL_FRAMEBUFFER, RendererId);

		OpenGLResourceManager* rm = (OpenGLResourceManager*)ResourceManager::Instance;

		uint32_t index = 0;

		for (const auto& colorTarget : ColorTargets)
		{
			if (colorTarget.IsValid())
			{
				OpenGLTexture* colorTexture = rm->GetTexture(colorTarget);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index++, GL_TEXTURE_2D, colorTexture->RendererId, 0);
			}
		}

		if (DepthTarget.IsValid())
		{
			OpenGLTexture* depthTexture = rm->GetTexture(DepthTarget);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture->RendererId, 0);
		}

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			assert(false);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}
