#pragma once

#include "Renderer\Enums.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

#include <cstdint>

namespace HBL2
{
	namespace OpenGLUtils
	{
		GLenum VertexFormatToGLenum(VertexFormat vertexFormat);
		uint32_t VertexFormatSize(VertexFormat vertexFormat);
		GLenum FormatToGLenum(Format format);
		GLenum FilterToGLenum(Filter filter);
		GLenum WrapToGLenum(Wrap wrap);
	}
}