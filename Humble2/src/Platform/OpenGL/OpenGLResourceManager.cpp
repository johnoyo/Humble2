#include "OpenGLResourceManager.h"

namespace HBL2
{
	void OpenGLResourceManager::Initialize()
	{
		m_TexturePool.Initialize(128);
		m_BufferPool.Initialize(512);
		m_ShaderPool.Initialize(64);
		m_FrameBufferPool.Initialize(32);
		m_BindGroupPool.Initialize(64);
		m_BindGroupLayoutPool.Initialize(32);
		m_RenderPassPool.Initialize(32);
		m_RenderPassLayoutPool.Initialize(32);
	}
	void OpenGLResourceManager::Clean()
	{
	}

	// Textures
	Handle<Texture> OpenGLResourceManager::CreateTexture(const TextureDescriptor&& desc)
	{
		return m_TexturePool.Insert(std::forward<const TextureDescriptor>(desc));
	}
	void OpenGLResourceManager::DeleteTexture(Handle<Texture> handle)
	{
		OpenGLTexture* texture = GetTexture(handle);
		if (texture != nullptr)
		{
			texture->Destroy();
			m_TexturePool.Remove(handle);
		}
	}
	void OpenGLResourceManager::UpdateTexture(Handle<Texture> handle, const Span<const std::byte>& bytes)
	{
		OpenGLTexture* texture = GetTexture(handle);
		if (texture != nullptr)
		{
			texture->Update(bytes);
		}
	}
	void OpenGLResourceManager::TransitionTextureLayout(CommandBuffer* commandBuffer, Handle<Texture> handle, TextureLayout currentLayout, TextureLayout newLayout, Handle<BindGroup> bindGroupHandle)
	{
	}
	glm::vec3 OpenGLResourceManager::GetTextureDimensions(Handle<Texture> handle)
	{
		OpenGLTexture* texture = GetTexture(handle);
		if (texture != nullptr)
		{
			return texture->Dimensions;
		}

		return { 0.f, 0.f, 0.f };
	}
	void* OpenGLResourceManager::GetTextureData(Handle<Texture> handle)
	{
		OpenGLTexture* texture = GetTexture(handle);
		if (texture != nullptr)
		{
			return texture->GetData();
		}

		return nullptr;
	}
	OpenGLTexture* OpenGLResourceManager::GetTexture(Handle<Texture> handle) const
	{
		return m_TexturePool.Get(handle);
	}

	// Buffers
	Handle<Buffer> OpenGLResourceManager::CreateBuffer(const BufferDescriptor&& desc)
	{
		return m_BufferPool.Insert(std::forward<const BufferDescriptor>(desc));
	}
	void OpenGLResourceManager::DeleteBuffer(Handle<Buffer> handle)
	{
		OpenGLBuffer* buffer = GetBuffer(handle);
		if (buffer != nullptr)
		{
			buffer->Destroy();
			m_BufferPool.Remove(handle);
		}
	}
	void OpenGLResourceManager::ReAllocateBuffer(Handle<Buffer> handle, uint32_t currentOffset)
	{
		OpenGLBuffer* buffer = GetBuffer(handle);
		buffer->ReAllocate(currentOffset);
	}
	void* OpenGLResourceManager::GetBufferData(Handle<Buffer> handle)
	{
		return m_BufferPool.Get(handle)->Data;
	}
	void OpenGLResourceManager::SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData)
	{
		OpenGLBuffer* openGLBuffer = GetBuffer(buffer);
		openGLBuffer->Data = (void*)((char*)newData + offset);
	}
	void OpenGLResourceManager::SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData)
	{
		OpenGLBindGroup* openGLBindGroup = GetBindGroup(bindGroup);
		if (bufferIndex < openGLBindGroup->Buffers.size())
		{
			SetBufferData(openGLBindGroup->Buffers[bufferIndex].buffer, openGLBindGroup->Buffers[bufferIndex].byteOffset, newData);
		}
	}
	void OpenGLResourceManager::MapBufferData(Handle<Buffer> buffer, intptr_t offset, intptr_t size)
	{
		OpenGLBuffer* openGLBuffer = GetBuffer(buffer);

		if (openGLBuffer == nullptr)
		{
			return;
		}

		glBindBuffer(openGLBuffer->Usage, openGLBuffer->RendererId);

		void* ptr = glMapBufferRange(openGLBuffer->Usage, offset, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);

		if (ptr == nullptr)
		{
			HBL2_CORE_ERROR("An error occured when trying to map data of buffer: {}.", openGLBuffer->DebugName);
			return;
		}

		memcpy(ptr, (void*)((char*)openGLBuffer->Data + offset), size);
		glUnmapBuffer(openGLBuffer->Usage);

		glBindBuffer(openGLBuffer->Usage, 0);
	}
	OpenGLBuffer* OpenGLResourceManager::GetBuffer(Handle<Buffer> handle) const
	{
		return m_BufferPool.Get(handle);
	}

	// Framebuffers
	Handle<FrameBuffer> OpenGLResourceManager::CreateFrameBuffer(const FrameBufferDescriptor&& desc)
	{
		return m_FrameBufferPool.Insert(std::forward<const FrameBufferDescriptor>(desc));
	}
	void OpenGLResourceManager::DeleteFrameBuffer(Handle<FrameBuffer> handle)
	{
		OpenGLFrameBuffer* framebuffer = GetFrameBuffer(handle);
		if (framebuffer != nullptr)
		{
			framebuffer->Destroy();
			m_FrameBufferPool.Remove(handle);
		}
	}
	void OpenGLResourceManager::ResizeFrameBuffer(Handle<FrameBuffer> handle, uint32_t width, uint32_t height)
	{
		if (!handle.IsValid())
		{
			return;
		}

		OpenGLFrameBuffer* frameBuffer = GetFrameBuffer(handle);
		frameBuffer->Resize(width, height);
	}
	OpenGLFrameBuffer* OpenGLResourceManager::GetFrameBuffer(Handle<FrameBuffer> handle) const
	{
		return m_FrameBufferPool.Get(handle);
	}

	// Shaders
	Handle<Shader> OpenGLResourceManager::CreateShader(const ShaderDescriptor&& desc)
	{
		return m_ShaderPool.Insert(std::forward<const ShaderDescriptor>(desc));
	}
	void OpenGLResourceManager::RecompileShader(Handle<Shader> handle, const ShaderDescriptor&& desc)
	{
		OpenGLShader* shader = GetShader(handle);
		if (shader != nullptr)
		{
			shader->Destroy();
			shader->Recompile(std::forward<const ShaderDescriptor>(desc));
		}
	}
	void OpenGLResourceManager::DeleteShader(Handle<Shader> handle)
	{
		OpenGLShader* shader = GetShader(handle);
		if (shader != nullptr)
		{
			shader->Destroy();
			m_ShaderPool.Remove(handle);
		}
	}
	uint64_t OpenGLResourceManager::GetOrAddShaderVariant(Handle<Shader> handle, const ShaderDescriptor::RenderPipeline::PackedVariant& variantDesc)
	{
		return variantDesc.Key();
	}
	OpenGLShader* OpenGLResourceManager::GetShader(Handle<Shader> handle) const
	{
		return m_ShaderPool.Get(handle);
	}

	// BindGroups
	Handle<BindGroup> OpenGLResourceManager::CreateBindGroup(const BindGroupDescriptor&& desc)
	{
		// FIXME: When shared between multiple bindgroups and we delete it once, the other references become invalid!
		#if 0
					// Caching mechanism so that materials with the same resources, use the same bind group.
		uint16_t index = 0;

		for (const auto& bindGroup : m_BindGroupPool.GetDataPool())
		{
			uint64_t descriptorHash = ResourceManager::Instance->GetBindGroupHash(desc);

			uint64_t hash = CalculateBindGroupHash(&bindGroup);

			if (descriptorHash == hash)
			{
				return m_BindGroupPool.GetHandleFromIndex(index);
			}

			index++;
		}
		#endif
		return m_BindGroupPool.Insert(std::forward<const BindGroupDescriptor>(desc));
	}
	void OpenGLResourceManager::DeleteBindGroup(Handle<BindGroup> handle)
	{
		OpenGLBindGroup* bindGroup = GetBindGroup(handle);
		if (bindGroup != nullptr)
		{
			bindGroup->Destroy();
			m_BindGroupPool.Remove(handle);
		}
	}
	void OpenGLResourceManager::UpdateBindGroup(Handle<BindGroup> handle) {}
	uint64_t OpenGLResourceManager::GetBindGroupHash(Handle<BindGroup> handle)
	{
		return CalculateBindGroupHash(GetBindGroup(handle));
	}
	OpenGLBindGroup* OpenGLResourceManager::GetBindGroup(Handle<BindGroup> handle) const
	{
		return m_BindGroupPool.Get(handle);
	}
	uint64_t OpenGLResourceManager::CalculateBindGroupHash(const OpenGLBindGroup* bindGroup)
	{
		if (bindGroup == nullptr)
		{
			return 0;
		}

		uint64_t hash = 0;

		for (const auto& bufferEntry : bindGroup->Buffers)
		{
			hash += bufferEntry.buffer.HashKey() + typeid(Buffer).hash_code();;
			hash += bufferEntry.byteOffset;
			hash += bufferEntry.range;
		}

		for (const auto texture : bindGroup->Textures)
		{
			hash += texture.HashKey() + typeid(Texture).hash_code();
		}

		hash += bindGroup->BindGroupLayout.HashKey() + typeid(BindGroupLayout).hash_code();

		return hash;
	}

	// BindGroupsLayouts
	Handle<BindGroupLayout> OpenGLResourceManager::CreateBindGroupLayout(const BindGroupLayoutDescriptor&& desc)
	{
		return m_BindGroupLayoutPool.Insert(std::forward<const BindGroupLayoutDescriptor>(desc));
	}
	void OpenGLResourceManager::DeleteBindGroupLayout(Handle<BindGroupLayout> handle)
	{
		m_BindGroupLayoutPool.Remove(handle);
	}
	OpenGLBindGroupLayout* OpenGLResourceManager::GetBindGroupLayout(Handle<BindGroupLayout> handle) const
	{
		return m_BindGroupLayoutPool.Get(handle);
	}

	// RenderPass
	Handle<RenderPass> OpenGLResourceManager::CreateRenderPass(const RenderPassDescriptor&& desc)
	{
		return m_RenderPassPool.Insert(std::forward<const RenderPassDescriptor>(desc));
	}
	void OpenGLResourceManager::DeleteRenderPass(Handle<RenderPass> handle)
	{
		m_RenderPassPool.Remove(handle);
	}
	OpenGLRenderPass* OpenGLResourceManager::GetRenderPass(Handle<RenderPass> handle) const
	{
		return m_RenderPassPool.Get(handle);
	}

	// RenderPassLayouts
	Handle<RenderPassLayout> OpenGLResourceManager::CreateRenderPassLayout(const RenderPassLayoutDescriptor&& desc)
	{
		return m_RenderPassLayoutPool.Insert(std::forward<const RenderPassLayoutDescriptor>(desc));
	}
	void OpenGLResourceManager::DeleteRenderPassLayout(Handle<RenderPassLayout> handle)
	{
		m_RenderPassLayoutPool.Remove(handle);
	}
	OpenGLRenderPassLayout* OpenGLResourceManager::GetRenderPassLayout(Handle<RenderPassLayout> handle) const
	{
		return m_RenderPassLayoutPool.Get(handle);
	}
}
