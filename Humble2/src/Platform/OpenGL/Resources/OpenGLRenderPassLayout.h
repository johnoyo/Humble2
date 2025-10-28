#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

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
	struct OpenGLRenderPassLayout
	{
		OpenGLRenderPassLayout() = default;
		OpenGLRenderPassLayout(const RenderPassLayoutDescriptor&& desc)
		{
			if (desc.subPasses.Size() > 0)
			{
				Pass = desc.subPasses[0]; // NOTE: There is only support for one subpass for now.
			}
		}

		const char* DebugName = "";
		RenderPassLayoutDescriptor::SubPass Pass;
	};
}