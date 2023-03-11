#pragma once

#include "..\..\Humble2\Renderer\RendererAPI.h"
#include "../../Platform/OpenGL/OpenGLShader.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <emscripten\emscripten.h>
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
		virtual void Initialize() override;
		virtual void DrawQuad() override;
		virtual void SetViewport(GLint x, GLint y, GLsizei width, GLsizei height) override;
	};
}