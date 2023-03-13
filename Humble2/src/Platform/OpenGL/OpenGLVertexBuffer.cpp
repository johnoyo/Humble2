#include "OpenGLVertexBuffer.h"

namespace HBL
{
	OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size) : m_TotalSize(size)
	{
		m_Buffer = new Buffer[m_TotalSize];

		glGenBuffers(1, &m_VertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, m_TotalSize, nullptr, GL_DYNAMIC_DRAW);

		// Vertex attrib positions.
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct Buffer), (const void*)offsetof(Buffer, Position));

		// Vertex attrib colors.
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(struct Buffer), (const void*)offsetof(Buffer, Color));

		// Vertex attrib texture coordinates.
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(struct Buffer), (const void*)offsetof(Buffer, TextureCoord));

		// Vertex attrib texture id.
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(struct Buffer), (const void*)offsetof(Buffer, TextureID));
	}

	void OpenGLVertexBuffer::SetData()
	{
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Buffer) * BatchSize, m_Buffer);
	}

	void OpenGLVertexBuffer::Bind()
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(struct Buffer), (const void*)offsetof(Buffer, Position));
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(struct Buffer), (const void*)offsetof(Buffer, Color));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(struct Buffer), (const void*)offsetof(Buffer, TextureCoord));
		glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(struct Buffer), (const void*)offsetof(Buffer, TextureID));
	}

	void OpenGLVertexBuffer::UnBind()
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}