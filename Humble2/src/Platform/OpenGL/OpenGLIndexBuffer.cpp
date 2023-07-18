#include "OpenGLIndexBuffer.h"

namespace HBL2
{
	OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t size, bool generated) : m_Size(size), m_Ganerated(generated)
	{
		if (generated)
		{
			GenerateIndeces(m_Size);
		}
		else
		{
			m_Size = size;
			m_Indeces = new uint32_t[m_Size];

			glGenBuffers(1, &m_IndexBufferID);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBufferID);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Size * sizeof(GLuint), m_Indeces, GL_STATIC_DRAW);
		}
	}

	void OpenGLIndexBuffer::GenerateIndeces(uint32_t size)
	{
		m_Size = size;

		m_Indeces = new uint32_t[m_Size * 6U];

		int w = 0;
		for (int k = 0; k < m_Size * 6; k += 6)
		{
			m_Indeces[m_Index++] = 0 + w;
			m_Indeces[m_Index++] = 3 + w;
			m_Indeces[m_Index++] = 2 + w;
			m_Indeces[m_Index++] = 2 + w;
			m_Indeces[m_Index++] = 1 + w;
			m_Indeces[m_Index++] = 0 + w;
			w += 4;
		}

		glGenBuffers(1, &m_IndexBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBufferID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Size * 6U * sizeof(GLuint), m_Indeces, GL_STATIC_DRAW);
	}

	void OpenGLIndexBuffer::Bind()
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IndexBufferID);
	}

	void OpenGLIndexBuffer::UnBind()
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void OpenGLIndexBuffer::SetData(uint32_t batchSize)
	{
		if (m_Ganerated)
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, batchSize * 6U * sizeof(GLuint), m_Indeces, GL_STATIC_DRAW);
		else
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, batchSize * sizeof(GLuint), m_Indeces, GL_STATIC_DRAW);
	}

	void OpenGLIndexBuffer::Invalidate(uint32_t size)
	{
		Clean();
		GenerateIndeces(size);
		SetData(size);
	}

	uint32_t* OpenGLIndexBuffer::GetHandle()
	{
		return m_Indeces;
	}

	void OpenGLIndexBuffer::Clean()
	{
		delete[] m_Indeces;

		glDeleteBuffers(1, &m_IndexBufferID);

		m_IndexBufferID = 0;
		m_Index = 0;
		m_Size = 0;
	}
}