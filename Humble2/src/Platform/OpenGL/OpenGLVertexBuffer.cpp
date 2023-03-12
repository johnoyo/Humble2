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
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Buffer) * m_BatchSize, m_Buffer);
	}

	void OpenGLVertexBuffer::Bind()
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
	}

	void OpenGLVertexBuffer::UnBind()
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void OpenGLVertexBuffer::AppendQuad(glm::vec3& position, float rotation, glm::vec3& scale, glm::vec4& color, float textureID)
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
			* glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(scale.x, scale.y, 1.0f));

		glm::vec4 quadVertexPosition[4];

		quadVertexPosition[0] = { -0.5f, 0.5f, 0.0f, 1.0f };
		quadVertexPosition[1] = {  0.5f, 0.5f, 0.0f, 1.0f };
		quadVertexPosition[2] = {  0.5f,-0.5f, 0.0f, 1.0f };
		quadVertexPosition[3] = { -0.5f,-0.5f, 0.0f, 1.0f };

		m_Buffer[m_BatchSize].Position = transform * quadVertexPosition[0];
		m_Buffer[m_BatchSize].Color = color;
		m_Buffer[m_BatchSize].TextureCoord = { 0.0f, 1.0f };
		m_Buffer[m_BatchSize].TextureID = textureID;
		m_BatchSize++;

		m_Buffer[m_BatchSize].Position = transform * quadVertexPosition[1];
		m_Buffer[m_BatchSize].Color = color;
		m_Buffer[m_BatchSize].TextureCoord = { 1.0f, 1.0f };
		m_Buffer[m_BatchSize].TextureID = textureID;
		m_BatchSize++;

		m_Buffer[m_BatchSize].Position = transform * quadVertexPosition[2];
		m_Buffer[m_BatchSize].Color = color;
		m_Buffer[m_BatchSize].TextureCoord = { 1.0f, 0.0f };
		m_Buffer[m_BatchSize].TextureID = textureID;
		m_BatchSize++;

		m_Buffer[m_BatchSize].Position = transform * quadVertexPosition[3];
		m_Buffer[m_BatchSize].Color = color;
		m_Buffer[m_BatchSize].TextureCoord = { 0.0f, 0.0f };
		m_Buffer[m_BatchSize].TextureID = textureID;
		m_BatchSize++;
	}

	void OpenGLVertexBuffer::Reset()
	{
		m_BatchSize = 0;
	}

	uint32_t OpenGLVertexBuffer::GetBatchSize()
	{
		return m_BatchSize;
	}

	Buffer* OpenGLVertexBuffer::GetHandle()
	{
		return m_Buffer;
	}
}