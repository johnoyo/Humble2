#pragma once

#include "../../Humble2/Renderer/IndexBuffer.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

namespace HBL
{
	class OpenGLIndexBuffer final : public IndexBuffer
	{
	public:
		OpenGLIndexBuffer(uint32_t size);
		virtual ~OpenGLIndexBuffer() {}

		virtual void Bind() override;
		virtual void UnBind() override;

		void SetData(uint32_t batchSize);
	private:
		uint32_t m_IndexBufferID = 0;
		uint32_t m_Index = 0;
		uint32_t m_Size = 0;
		uint32_t* m_Indeces;
	};
}