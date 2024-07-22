#pragma once

#include "Base.h"
#include "Renderer/VertexBuffer.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

namespace HBL2
{
	class OpenGLVertexBuffer final : public VertexBuffer
	{
	public:
		OpenGLVertexBuffer(uint32_t size, VertexBufferLayout& layout);
		OpenGLVertexBuffer(HBL::Buffer* handle, uint32_t size, VertexBufferLayout& layout);
		virtual ~OpenGLVertexBuffer() {}

		virtual void Bind() override;
		virtual void UnBind() override;
		virtual void SetData() override;
		virtual void Clean() override;
	private:
		uint32_t m_VertexBufferID = 0;
		uint32_t m_TotalSize = 0;
		bool m_DynamicDraw = false;
		VertexBufferLayout m_Layout;
		std::vector<GLenum> m_Types;
		std::vector<GLsizei> m_Offsets;
	};
}