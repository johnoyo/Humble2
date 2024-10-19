#include "OpenGLBuffer.h"

#include "Platform\OpenGL\OpenGLResourceManager.h"

namespace HBL2
{
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
			glVertexAttribPointer(bufferIndex + j, size, type, GL_FALSE, stride, (const void*)offset);
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
		}
	}

	void OpenGLBuffer::BindStorageBuffer(Handle<Material> material, uint32_t bufferIndex)
	{
		OpenGLResourceManager* rm = (OpenGLResourceManager*)ResourceManager::Instance;
	}
}
