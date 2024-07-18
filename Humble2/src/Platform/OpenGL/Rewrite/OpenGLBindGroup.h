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
	struct OpenGLBindGroup
	{
		OpenGLBindGroup() = default;
		OpenGLBindGroup(BindGroupDescriptor& desc) {}

		const char* DebugName = "";
	};
}