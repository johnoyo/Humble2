#pragma once

#include "Base.h"
#include "Renderer\FrameBuffer.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

namespace HBL2
{
	class OpenGLFrameBuffer final : public FrameBuffer
	{
	public:
		OpenGLFrameBuffer(const FrameBufferSpecification& spec);
		virtual ~OpenGLFrameBuffer() {}

		virtual void Invalidate() override;
		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual void Bind() override;
		virtual void UnBind() override;

		virtual void Clean() override;

		virtual uint32_t GetColorAttachmentID() const override { return m_ColorAttachment; }
		virtual uint32_t GetDepthAttachmentID() const override { return m_DepthAttachment; }

		virtual FrameBufferSpecification& GetSpecification() override { return m_Specification; }

	private:
		uint32_t m_RendererID = 0;
		uint32_t m_ColorAttachment = 0;
		uint32_t m_DepthAttachment = 0;
		FrameBufferSpecification m_Specification = {};
	};
}