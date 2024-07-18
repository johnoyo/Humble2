#pragma once

#include "Base.h"
#include "Renderer\Rewrite\TypeDescriptors.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>

namespace HBL2
{
	struct OpenGLFrameBuffer
	{
		OpenGLFrameBuffer() = default;
		OpenGLFrameBuffer(const FrameBufferDescriptor& desc)
		{
			DebugName = desc.debugName;
			Width = desc.width;
			Height = desc.height;

			Invalidate();
		}

		void Resize(uint32_t width, uint32_t height)
		{
			Width = width;
			Height = height;

			Invalidate();
		}

		void Invalidate()
		{
			if (RendererId)
			{
				glDeleteFramebuffers(1, &RendererId);
				glDeleteTextures(1, &ColorAttachmentId);
				glDeleteTextures(1, &DepthAttachmentId);
			}

			// Build the texture that will serve as the color attachment for the framebuffer.
			glGenTextures(1, &ColorAttachmentId);
			glBindTexture(GL_TEXTURE_2D, ColorAttachmentId);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifndef EMSCRIPTEN
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
#endif
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Width, Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

			glBindTexture(GL_TEXTURE_2D, 0);

			// Build the texture that will serve as the depth attachment for the framebuffer.
			glGenTextures(1, &DepthAttachmentId);
			glBindTexture(GL_TEXTURE_2D, DepthAttachmentId);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, Width, Height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);

			glBindTexture(GL_TEXTURE_2D, 0);

			// Build the framebuffer.
			glGenFramebuffers(1, &RendererId);
			glBindFramebuffer(GL_FRAMEBUFFER, RendererId);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ColorAttachmentId, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, DepthAttachmentId, 0);

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE)
			{
				assert(false);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		const char* DebugName = "";
		uint32_t RendererId = 0;
		uint32_t ColorAttachmentId = 0;
		uint32_t DepthAttachmentId = 0;
		uint32_t Width = 0;
		uint32_t Height = 0;
	};
}