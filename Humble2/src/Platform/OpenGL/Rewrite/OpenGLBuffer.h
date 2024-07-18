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
	struct OpenGLBuffer
	{
		OpenGLBuffer() = default;
		OpenGLBuffer(const BufferDescriptor& desc)
		{
			DebugName = desc.debugName;
			ByteSize = desc.byteSize;
			Data = desc.initialData;

			glGenBuffers(1, &RendererId);
			glBindBuffer(GL_ARRAY_BUFFER, RendererId);
			glBufferData(GL_ARRAY_BUFFER, ByteSize, Data, GL_STATIC_DRAW);
		}

		const char* DebugName = "";
		uint32_t RendererId = 0;
		uint32_t ByteSize = 0;
		uint32_t BatchSize = 1;
		void* Data;
	};
}