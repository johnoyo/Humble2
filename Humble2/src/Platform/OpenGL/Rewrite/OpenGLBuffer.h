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
	struct OpenGLBuffer
	{
		OpenGLBuffer() = default;
		OpenGLBuffer(const BufferDescriptor&& desc)
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

		void ReAllocate(uint32_t currentOffset)
		{
			if (UsageHint == GL_STATIC_DRAW)
			{
				HBL2_CORE_WARN("Buffer with name: {0} is set for static usage, re-allocating will have no effect", DebugName);
				return;
			}

			GLuint newRendererId;
			glGenBuffers(1, &newRendererId);
			glBindBuffer(Usage, newRendererId);
			glBufferData(Usage, ByteSize * 2, nullptr, GL_DYNAMIC_DRAW);

			// Copy existing data to the new buffer
			glBindBuffer(GL_COPY_READ_BUFFER, RendererId);
			glBindBuffer(GL_COPY_WRITE_BUFFER, newRendererId);
			glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, currentOffset);

			// Delete the old buffer
			glDeleteBuffers(1, &RendererId);

			// Update buffer and size references
			RendererId = newRendererId;
			ByteSize = ByteSize * 2;

			// Unbind buffers
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
			glBindBuffer(GL_COPY_READ_BUFFER, 0);
			glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
		}

		const char* DebugName = "";
		uint32_t RendererId = 0;
		uint32_t ByteSize = 0;
		uint32_t BatchSize = 1;
		uint32_t Usage = GL_ARRAY_BUFFER;
		uint32_t UsageHint = GL_STATIC_DRAW;
		void* Data = nullptr;
	};
}