#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\OpenGL\OpenGLCommon.h"

#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>

namespace HBL2
{
	struct Material;

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
			glBufferData(Usage, (GLsizeiptr)(ByteSize * 2), nullptr, GL_DYNAMIC_DRAW);

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

		void SetData(void* newData, intptr_t offset = 0)
		{
			Data = (void*)((char*)newData + offset);
		}

		void Write(intptr_t offset = 0, GLsizeiptr size = 0)
		{
			if (UsageHint == GL_STATIC_DRAW)
			{
				HBL2_CORE_WARN("Buffer with name: {0} is set for static usage, updating will have no effect", DebugName);
				return;
			}

			if (size == 0)
			{
				size = ByteSize;
			}

			glBindBuffer(Usage, RendererId);
			glBufferSubData(Usage, offset, size, Data);
		}

		void Bind(Handle<Material> material = {}, uint32_t bufferIndex = 0, intptr_t offset = 0, uint32_t size = 0)
		{
			switch (Usage)
			{
			case GL_ELEMENT_ARRAY_BUFFER:
				BindIndexBuffer();
				break;
			case GL_ARRAY_BUFFER:
				BindVertexBuffer(material, bufferIndex);
				break;
			case GL_UNIFORM_BUFFER:
				BindUniformBuffer(material, bufferIndex, offset, size);
				break;
			case GL_SHADER_STORAGE_BUFFER:
				BindStorageBuffer(material, bufferIndex);
				break;
			}
		}

		void Destroy()
		{
			glDeleteBuffers(1, &RendererId);
		}

		const char* DebugName = "";
		GLuint RendererId = 0;
		uint32_t ByteSize = 0;
		uint32_t Usage = GL_ARRAY_BUFFER;
		uint32_t UsageHint = GL_STATIC_DRAW;
		void* Data = nullptr;

	private:
		void BindIndexBuffer();
		void BindVertexBuffer(Handle<Material> material, uint32_t bufferIndex);
		void BindUniformBuffer(Handle<Material> material, uint32_t bufferIndex, intptr_t offset, uint32_t size);
		void BindStorageBuffer(Handle<Material> material, uint32_t bufferIndex);
	};
}