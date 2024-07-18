#pragma once

#include "Base.h"
#include "Renderer\Rewrite\TypeDescriptors.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>

namespace HBL2
{
	struct OpenGLTexture
	{
		OpenGLTexture() = default;
		OpenGLTexture(const TextureDescriptor& desc) {}

		const char* DebugName = "";
		uint32_t RendererId = 0;
		glm::vec3 Dimensions = glm::vec3(0.0);
		uint32_t Format = 0;
		const char* Path = "";
	};
}