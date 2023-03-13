#pragma once

#include "../../Humble2/Renderer/RendererAPI.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#include "OpenGLDebug.h"
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

namespace HBL
{
	class OpenGLRendererAPI final : public RendererAPI
	{
	public:
		OpenGLRendererAPI();
		virtual void SetViewport(GLint x, GLint y, GLsizei width, GLsizei height) override;
		virtual void ClearScreen(glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }) override;
		virtual void Submit(VertexBuffer* vertexBuffer) override;
	};
}