#include "OpenGLBindGroup.h"

#include "Platform\OpenGL\OpenGLResourceManager.h"
#include "Utilities\JobSystem.h"

namespace HBL2
{
	OpenGLBindGroup::OpenGLBindGroup(const BindGroupDescriptor&& desc)
	{
		DebugName = desc.debugName;
		Buffers = { desc.buffers.begin(), desc.buffers.end() };
		Textures = { desc.textures.begin(), desc.textures.end() };
		BindGroupLayout = desc.layout;

		auto* rm = (OpenGLResourceManager*)ResourceManager::Instance;

		OpenGLBindGroupLayout* bindGroupLayout = rm->GetBindGroupLayout(BindGroupLayout);

		for (int i = 0; i < Buffers.size(); i++)
		{
			OpenGLBuffer* buffer = rm->GetBuffer(Buffers[i].buffer);

			switch (bindGroupLayout->BufferBindings[i].type)
			{
			case BufferBindingType::UNIFORM:
				glBindBufferBase(GL_UNIFORM_BUFFER, bindGroupLayout->BufferBindings[i].slot, buffer->RendererId);
				break;
			case BufferBindingType::UNIFORM_DYNAMIC_OFFSET:
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
			OpenGLTexture* texture = rm->GetTexture(Textures[i].texture);

			// TODO: Handle textures.
			// ...
		}

		if (!JobSystem::Get().IsRenderThread())
		{
			glFlush();
		}
	}

	void OpenGLBindGroup::Set()
	{
		OpenGLResourceManager* rm = (OpenGLResourceManager*)ResourceManager::Instance;
		OpenGLBindGroupLayout* openGLBindGroupLayout = rm->GetBindGroupLayout(BindGroupLayout);

		for (int i = 0; i < Textures.size(); i++)
		{
			OpenGLTexture* openGLTexture = rm->GetTexture(Textures[i].texture);

			glActiveTexture(openGLBindGroupLayout->TextureBindings[i].slot + GL_TEXTURE0);
			glBindTexture(openGLTexture->TextureType, openGLTexture->ViewRendererId);
		}
	}

	void OpenGLBindGroup::Destroy()
	{
		OpenGLResourceManager* rm = (OpenGLResourceManager*)ResourceManager::Instance;
		
		for (int i = 0; i < Buffers.size(); i++)
		{
			// If the range is not 0, this means its a dynamic uniform buffer meaning that is shared across bindgroup, so do not delete.
			if (Buffers[i].range == 0)
			{
				rm->DeleteBuffer(Buffers[i].buffer);
			}
		}

		for (int i = 0; i < Textures.size(); i++)
		{
			// NOTE(John): Do not delete texture since we might use it else where.
			// rm->DeleteTexture(Textures[i]);
		}
	}
}

