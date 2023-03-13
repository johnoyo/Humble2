#pragma once

#include "../Humble2/Renderer/VertexArray.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

namespace HBL
{
	class OpenGLVertexArray final : public VertexArray
	{
	public:
		OpenGLVertexArray();
		virtual ~OpenGLVertexArray() {}

		virtual void Bind() override;
		virtual void UnBind() override;
		virtual std::vector<VertexBuffer*>& GetVertexBuffers() override;
		virtual IndexBuffer* GetIndexBuffer() override;
		virtual void AddVertexBuffer(VertexBuffer* vertexBuffer) override;
		virtual void SetIndexBuffer(IndexBuffer* indexBuffer) override;
	private:
		uint32_t m_VertexArrayObject = 0;
		IndexBuffer* m_IndexBuffer = nullptr;
		std::vector<VertexBuffer*> m_VertexBuffers;
	};
}