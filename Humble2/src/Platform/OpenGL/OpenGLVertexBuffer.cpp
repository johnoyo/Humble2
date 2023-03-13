#include "OpenGLVertexBuffer.h"

namespace HBL
{
	OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size, VertexBufferLayout& layout) : m_TotalSize(size), m_Layout(layout)
	{
		m_Buffer = new Buffer[m_TotalSize];

		glGenBuffers(1, &m_VertexBufferID);
		glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
		glBufferData(GL_ARRAY_BUFFER, m_TotalSize * sizeof(struct Buffer), nullptr, GL_DYNAMIC_DRAW);

		GLsizei offset = 0;

		for (auto& element : m_Layout.GetVertexBufferElements())
		{
			GLenum type = 0;
			GLuint sizeOfType = 0;

			switch (element.Type)
			{
			case Type::FLOAT:
				type = GL_FLOAT;
				sizeOfType = 4;
				break;
			case Type::UNSIGNED_INT:
				type = GL_UNSIGNED_INT;
				sizeOfType = 4;
				break;
			case Type::UNSIGNED_BYTE:
				type = GL_UNSIGNED_BYTE;
				sizeOfType = 1;
				break;
			}

			m_Types.push_back(type);
			m_Offsets.push_back(offset);

			glEnableVertexAttribArray(element.Index);
			glVertexAttribPointer(element.Index, element.Count, type, element.Normalized ? GL_TRUE : GL_FALSE, m_Layout.GetStride(), (const void*)offset);

			offset += element.Count * sizeOfType;
		}
	}

	void OpenGLVertexBuffer::SetData()
	{
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(struct Buffer) * BatchSize, m_Buffer);
	}

	void OpenGLVertexBuffer::Bind()
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);

		GLuint i = 0;
		for (auto& element : m_Layout.GetVertexBufferElements())
		{
			glEnableVertexAttribArray(element.Index);
			glVertexAttribPointer(element.Index, element.Count, m_Types[i], element.Normalized ? GL_TRUE : GL_FALSE, m_Layout.GetStride(), (const void*)m_Offsets[i]);
			i++;
		}
	}

	void OpenGLVertexBuffer::UnBind()
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}