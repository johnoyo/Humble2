#include "OpenGLBindGroup.h"

#include "OpenGLResourceManager.h"

namespace HBL2
{
	OpenGLBindGroup::OpenGLBindGroup(const BindGroupDescriptor& desc)
	{
		DebugName = desc.debugName;
		Buffers = desc.buffers;
		Textures = desc.textures;

		auto* rm = (OpenGLResourceManager*)ResourceManager::Instance;

		OpenGLBindGroupLayout* bindGroupLayout = rm->GetBindGroupLayout(desc.layout);

		for (int i = 0; i < Buffers.size(); i++)
		{
			OpenGLBuffer* buffer = rm->GetBuffer(Buffers[i].buffer);

			switch (bindGroupLayout->BufferBindings[i].type)
			{
			case BufferBindingType::UNIFORM:
				glBindBufferBase(GL_UNIFORM_BUFFER, bindGroupLayout->BufferBindings[i].slot, buffer->RendererId);
				break;
			case BufferBindingType::STORAGE:
				break;
			case BufferBindingType::READ_ONLY_STORAGE:
				break;
			default:
				break;
			}
		}

		for (int i = 0; i < Textures.size(); i++)
		{
			OpenGLTexture* buffer = rm->GetTexture(Textures[i]);

			// TODO: Handle textures.
			// ...
		}
	}
}

