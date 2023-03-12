#pragma once

#include "OpenGLShader.h"
#include "OpenGLVertexArray.h"
#include "OpenGLVertexBuffer.h"
#include "OpenGLIndexBuffer.h"

#include "../../Humble2/Renderer/RendererAPI.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

namespace HBL
{
	class OpenGLRendererAPI final : public RendererAPI
	{
	public:
		OpenGLRendererAPI();
		virtual void DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale) override;
		virtual void DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale, glm::vec4& color) override;
		virtual void DrawQuad(glm::vec3& position, float rotation, glm::vec3& scale, uint32_t textureID, glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }) override;
		virtual void SetViewport(GLint x, GLint y, GLsizei width, GLsizei height) override;
		virtual void ClearScreen(glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }) override;
		virtual void Flush() override;
	private:
		OpenGLVertexArray* m_VertexArray = nullptr;
		OpenGLVertexBuffer* m_VertexBuffer = nullptr;
		OpenGLIndexBuffer* m_IndexBuffer = nullptr;
	};
}