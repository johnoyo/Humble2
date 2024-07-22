#pragma once

#include "Renderer/RendererAPI.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#include "Platform\OpenGL\Rewrite\OpenGLDebug.h"
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

namespace HBL2
{
	class OpenGLRendererAPI final : public RendererAPI
	{
	public:
		OpenGLRendererAPI();
		virtual void SetViewport(GLint x, GLint y, GLsizei width, GLsizei height) override;
		virtual void ClearScreen(glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }) override;
		virtual void Draw(VertexBuffer* vertexBuffer) override;
		virtual void DrawIndexed(VertexBuffer* vertexBuffer) override;
	};
}