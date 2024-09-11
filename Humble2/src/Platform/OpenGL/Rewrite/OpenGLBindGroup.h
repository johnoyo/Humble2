#pragma once

#include "Base.h"
#include "Resources\Handle.h"
#include "Resources\TypeDescriptors.h"

#include "OpenGLBuffer.h"
#include "OpenGLTexture.h"
#include "OpenGLBindGroupLayout.h"

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
	class OpenGLResourceManager;

	struct OpenGLBindGroup
	{
		OpenGLBindGroup() = default;
		OpenGLBindGroup(const BindGroupDescriptor& desc);		

		const char* DebugName = "";
		std::vector<BindGroupDescriptor::BufferEntry> Buffers;
		std::vector<Handle<Texture>> Textures;
		Handle<BindGroupLayout> BindGroupLayout;
	};
}