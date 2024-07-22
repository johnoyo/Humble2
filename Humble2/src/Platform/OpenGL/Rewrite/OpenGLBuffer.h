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

			switch (desc.usage)
			{
			case HBL2::BufferUsage::MAP_READ:
				break;
			case HBL2::BufferUsage::MAP_WRITE:
				break;
			case HBL2::BufferUsage::COPY_SRC:
				break;
			case HBL2::BufferUsage::COPY_DST:
				break;
			case HBL2::BufferUsage::INDEX:
				Usage = GL_ELEMENT_ARRAY_BUFFER;
				break;
			case HBL2::BufferUsage::VERTEX:
				Usage = GL_ARRAY_BUFFER;
				break;
			case HBL2::BufferUsage::UNIFORM:
				Usage = GL_UNIFORM_BUFFER;
				break;
			case HBL2::BufferUsage::STORAGE:
				Usage = GL_SHADER_STORAGE_BUFFER;
				break;
			case HBL2::BufferUsage::INDIRECT:
				break;
			case HBL2::BufferUsage::QUERY_RESOLVE:
				break;
			default:
				break;
			}

			glGenBuffers(1, &RendererId);
			glBindBuffer(Usage, RendererId);

			switch (desc.usageHint)
			{
			case BufferUsageHint::STATIC:
				UsageHint = GL_STATIC_DRAW;
				glBufferData(Usage, ByteSize, Data, GL_STATIC_DRAW);
				break;
			case BufferUsageHint::DYNAMIC:
				UsageHint = GL_DYNAMIC_DRAW;
				glBufferData(Usage, ByteSize, nullptr, GL_DYNAMIC_DRAW);
				break;
			default:
				break;
			}
		}

		const char* DebugName = "";
		uint32_t RendererId = 0;
		uint32_t ByteSize = 0;
		uint32_t BatchSize = 1;
		uint32_t Usage = GL_ARRAY_BUFFER;
		uint32_t UsageHint = GL_ARRAY_BUFFER;
		void* Data;
	};
}