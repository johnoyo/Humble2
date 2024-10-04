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
	struct OpenGLBindGroupLayout
	{
		OpenGLBindGroupLayout() = default;
		OpenGLBindGroupLayout(const BindGroupLayoutDescriptor&& desc)
		{
			DebugName = desc.debugName;
			BufferBindings = desc.bufferBindings;
			TextureBindings = desc.textureBindings;
		}

		const char* DebugName = "";
		std::vector<BindGroupLayoutDescriptor::BufferBinding> BufferBindings;
		std::vector<BindGroupLayoutDescriptor::TextureBinding> TextureBindings;
	};
}