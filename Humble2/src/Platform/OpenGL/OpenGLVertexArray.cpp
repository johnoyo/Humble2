#include "OpenGLVertexArray.h"
#include <iostream>

namespace HBL
{
	OpenGLVertexArray::OpenGLVertexArray()
	{
		glGenVertexArrays(1, &m_VertexArrayObject);
		glBindVertexArray(m_VertexArrayObject);
	}

	void OpenGLVertexArray::Bind()
	{
		glBindVertexArray(m_VertexArrayObject);
	}

	void OpenGLVertexArray::UnBind()
	{
		glBindVertexArray(0);
	}

	std::vector<VertexBuffer*>& OpenGLVertexArray::GetVertexBuffers()
	{
		return m_VertexBuffers;
	}

	IndexBuffer* OpenGLVertexArray::GetIndexBuffer()
	{
		return m_IndexBuffer;
	}

	void OpenGLVertexArray::AddVertexBuffer(VertexBuffer* vertexBuffer)
	{
		vertexBuffer->BatchIndex = m_VertexBuffers.size();
		m_VertexBuffers.push_back(vertexBuffer);
	}

	void OpenGLVertexArray::SetIndexBuffer(IndexBuffer* indexBuffer)
	{
		m_IndexBuffer = indexBuffer;
	}
}