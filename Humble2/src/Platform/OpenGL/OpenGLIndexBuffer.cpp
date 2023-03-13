#include "OpenGLIndexBuffer.h"

namespace HBL
{
	OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t size) : m_Size(size)
	{
		m_Indeces = new uint32_t[(m_Size / 4U) * 6U];

		int w = 0;
		for (int k = 0; k < (m_Size / 4) * 6; k += 6)
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
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (m_Size / 4U) * 6U * sizeof(unsigned int), m_Indeces, GL_STATIC_DRAW);
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
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, (batchSize / 4U) * 6U * sizeof(unsigned int), m_Indeces, GL_STATIC_DRAW);
	}
}