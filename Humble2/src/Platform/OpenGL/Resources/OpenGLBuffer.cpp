#include "OpenGLBuffer.h"

#include "Platform\OpenGL\OpenGLResourceManager.h"

namespace HBL2
{
	OpenGLBuffer::OpenGLBuffer(const BufferDescriptor&& desc)
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

	void OpenGLBuffer::ReAllocate(uint32_t currentOffset)
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

	void OpenGLBuffer::SetData(void* newData, intptr_t offset)
	{
		Data = (void*)((char*)newData + offset);
	}

	void OpenGLBuffer::Write(intptr_t offset, GLsizeiptr size)
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

	void OpenGLBuffer::Bind(Handle<Material> material, uint32_t bufferIndex, intptr_t offset, uint32_t size)
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

	void OpenGLBuffer::Destroy()
	{
		glDeleteBuffers(1, &RendererId);
	}

	void OpenGLBuffer::BindIndexBuffer()
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RendererId);
	}

	void OpenGLBuffer::BindVertexBuffer(Handle<Material> material, uint32_t bufferIndex)
	{
		OpenGLResourceManager* rm = (OpenGLResourceManager*)ResourceManager::Instance;
		Material* mat = rm->GetMaterial(material);

		HBL2_CORE_ASSERT(mat != nullptr, "Material handle is invalid.");

		OpenGLShader* shader = rm->GetShader(mat->Shader);
		HBL2_CORE_ASSERT(shader != nullptr, "Shader handle for material \"" + std::string(mat->DebugName) + "\" is invalid.");

		const auto& binding = shader->VertexBufferBindings[bufferIndex];

		for (int j = 0; j < binding.attributes.size(); j++)
		{
			auto& attribute = binding.attributes[j];

			GLint size = OpenGLUtils::VertexFormatSize(attribute.format);
			GLenum type = OpenGLUtils::VertexFormatToGLenum(attribute.format);
			GLsizei stride = binding.byteStride;
			GLsizei offset = attribute.byteOffset;

			glBindBuffer(GL_ARRAY_BUFFER, RendererId);
			glEnableVertexAttribArray(bufferIndex + j);
			if (type == GL_FLOAT)
			{
				glVertexAttribPointer(bufferIndex + j, size, type, GL_FALSE, stride, (const void*)offset);
			}
			else
			{
				glVertexAttribIPointer(bufferIndex + j, size, type, stride, (const void*)offset);
			}
		}
	}

	void OpenGLBuffer::BindUniformBuffer(Handle<Material> material, uint32_t bufferIndex, intptr_t offset, uint32_t size)
	{
		OpenGLResourceManager* rm = (OpenGLResourceManager*)ResourceManager::Instance;
		Material* mat = rm->GetMaterial(material);

		HBL2_CORE_ASSERT(mat != nullptr, "Material handle is invalid.");

		OpenGLBindGroup* openGLBindGroup = rm->GetBindGroup(mat->BindGroup);
		OpenGLBindGroupLayout* bindGroupLayout = rm->GetBindGroupLayout(openGLBindGroup->BindGroupLayout);

		if (bufferIndex < openGLBindGroup->Buffers.size())
		{
			if (bindGroupLayout->BufferBindings[bufferIndex].type == BufferBindingType::UNIFORM_DYNAMIC_OFFSET)
			{
				glBindBufferRange(GL_UNIFORM_BUFFER, bindGroupLayout->BufferBindings[bufferIndex].slot, RendererId, offset, size);
			}
			else
			{
				glBindBufferBase(GL_UNIFORM_BUFFER, bindGroupLayout->BufferBindings[bufferIndex].slot, RendererId);
			}
		}
	}

	void OpenGLBuffer::BindStorageBuffer(Handle<Material> material, uint32_t bufferIndex)
	{
		OpenGLResourceManager* rm = (OpenGLResourceManager*)ResourceManager::Instance;
	}
}
