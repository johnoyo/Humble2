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
		OpenGLBuffer(const BufferDescriptor&& desc);

		void ReAllocate(uint32_t currentOffset);
		void SetData(void* newData, intptr_t offset = 0);
		void Write(intptr_t offset = 0, GLsizeiptr size = 0);

		void Bind();
		void Bind(Handle<Shader> shader, uint32_t bufferIndex);
		void Bind(Handle<BindGroup> bindGroup, uint32_t bufferIndex, intptr_t offset, uint32_t size);
		void Bind(Handle<Shader> shader, Handle<BindGroup> bindGroup, uint32_t bufferIndex, intptr_t offset, uint32_t size);

		void Destroy();

		const char* DebugName = "";
		GLuint RendererId = 0;
		uint32_t ByteSize = 0;
		GLuint Usage = GL_ARRAY_BUFFER;
		GLuint UsageHint = GL_STATIC_DRAW;
		void* Data = nullptr;

	private:
		void BindIndexBuffer();
		void BindVertexBuffer(Handle<Shader> shader, uint32_t bufferIndex);
		void BindUniformBuffer(Handle<BindGroup> bindGroup, uint32_t bufferIndex, intptr_t offset, uint32_t size);
		void BindStorageBuffer(Handle<BindGroup> bindGroup, uint32_t bufferIndex);
	};
}