#pragma once

#include "../../Humble2/Renderer/VertexBuffer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

namespace HBL
{
	struct Buffer
	{
		glm::vec2 Position;
		glm::vec4 Color;
		glm::vec2 TextureCoord;
		float TextureID;
	};

	class OpenGLVertexBuffer final : public VertexBuffer
	{
	public:
		OpenGLVertexBuffer(uint32_t size);
		virtual ~OpenGLVertexBuffer() {}

		virtual void SetData() override;
		virtual void Bind() override;
		virtual void UnBind() override;

		void AppendQuad(glm::vec3& position, float rotation, glm::vec3& scale, glm::vec4& color, float textureID);
		uint32_t GetBatchSize();
		void Reset();

		Buffer* GetHandle();
	private:
		uint32_t m_VertexBufferID = 0;
		uint32_t m_TotalSize = 0;
		uint32_t m_BatchSize = 0;
		uint32_t m_CurrentIndex = 0;
		Buffer* m_Buffer = nullptr;
	};
}