#pragma once

#include "../Base.h"
#include "../../Humble2/Renderer/VertexBuffer.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

namespace HBL
{
	class OpenGLVertexBuffer final : public VertexBuffer
	{
	public:
		OpenGLVertexBuffer(uint32_t size, VertexBufferLayout& layout);
		virtual ~OpenGLVertexBuffer() {}

		virtual void Bind() override;
		virtual void UnBind() override;
		virtual void SetData() override;
	private:
		uint32_t m_VertexBufferID = 0;
		uint32_t m_TotalSize = 0;
		VertexBufferLayout m_Layout;
		std::vector<GLenum> m_Types;
		std::vector<GLsizei> m_Offsets;
	};
}