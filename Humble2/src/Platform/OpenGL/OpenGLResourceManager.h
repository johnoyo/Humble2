#pragma once

#include "Resources\ResourceManager.h"

#include "Resources\Pool.h"
#include "Resources\Types.h"
#include "Resources\TypeDescriptors.h"

#include "Resources\OpenGLBuffer.h"
#include "Resources\OpenGLShader.h"
#include "Resources\OpenGLTexture.h"
#include "Resources\OpenGLFrameBuffer.h"
#include "Resources\OpenGLBindGroup.h"
#include "Resources\OpenGLBindGroupLayout.h"
#include "Resources\OpenGLRenderPass.h"
#include "Resources\OpenGLRenderPassLayout.h"

namespace HBL2
{
	class OpenGLResourceManager final : public ResourceManager
	{
	public:
		virtual ~OpenGLResourceManager() = default;

		virtual void Clean() override {}

		// Textures
		virtual Handle<Texture> CreateTexture(const TextureDescriptor&& desc) override
		{
			return m_TexturePool.Insert(std::forward<const TextureDescriptor>(desc));
		}
		virtual void DeleteTexture(Handle<Texture> handle) override
		{
			OpenGLTexture* texture = GetTexture(handle);
			if (texture != nullptr)
			{
				texture->Destroy();
				m_TexturePool.Remove(handle);
			}
		}
		virtual void UpdateTexture(Handle<Texture> handle, const Span<const std::byte>& bytes) override
		{
			OpenGLTexture* texture = GetTexture(handle);
			if (texture != nullptr)
			{
				texture->Update(bytes);
			}
		}
		virtual void TransitionTextureLayout(CommandBuffer* commandBuffer, Handle<Texture> handle, TextureLayout currentLayout, TextureLayout newLayout, Handle<BindGroup> bindGroupHandle) override {}
		virtual glm::vec3 GetTextureDimensions(Handle<Texture> handle) override
		{
			OpenGLTexture* texture = GetTexture(handle);
			if (texture != nullptr)
			{
				return texture->Dimensions;
			}

			return { 0.f, 0.f, 0.f };
		}
		OpenGLTexture* GetTexture(Handle<Texture> handle) const
		{
			return m_TexturePool.Get(handle);
		}

		// Buffers
		virtual Handle<Buffer> CreateBuffer(const BufferDescriptor&& desc) override
		{
			return m_BufferPool.Insert(std::forward<const BufferDescriptor>(desc));
		}
		virtual void DeleteBuffer(Handle<Buffer> handle) override
		{
			OpenGLBuffer* buffer = GetBuffer(handle);
			if (buffer != nullptr)
			{
				buffer->Destroy();
				m_BufferPool.Remove(handle);
			}
		}
		virtual void ReAllocateBuffer(Handle<Buffer> handle, uint32_t currentOffset) override
		{
			OpenGLBuffer* buffer = GetBuffer(handle);
			buffer->ReAllocate(currentOffset);
		}
		virtual void* GetBufferData(Handle<Buffer> handle) override
		{
			return m_BufferPool.Get(handle)->Data;
		}
		virtual void SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData) override
		{
			OpenGLBuffer* openGLBuffer = GetBuffer(buffer);
			openGLBuffer->Data = (void*)((char*)newData + offset);
		}
		virtual void SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData) override
		{
			OpenGLBindGroup* openGLBindGroup = GetBindGroup(bindGroup);
			if (bufferIndex < openGLBindGroup->Buffers.size())
			{
				SetBufferData(openGLBindGroup->Buffers[bufferIndex].buffer, openGLBindGroup->Buffers[bufferIndex].byteOffset, newData);
			}
		}
		virtual void MapBufferData(Handle<Buffer> buffer, intptr_t offset, intptr_t size) override
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
		OpenGLBuffer* GetBuffer(Handle<Buffer> handle) const
		{
			return m_BufferPool.Get(handle);
		}

		// Framebuffers
		virtual Handle<FrameBuffer> CreateFrameBuffer(const FrameBufferDescriptor&& desc) override
		{
			return m_FrameBufferPool.Insert(std::forward<const FrameBufferDescriptor>(desc));
		}
		virtual void DeleteFrameBuffer(Handle<FrameBuffer> handle) override
		{
			OpenGLFrameBuffer* framebuffer = GetFrameBuffer(handle);
			if (framebuffer != nullptr)
			{
				framebuffer->Destroy();
				m_FrameBufferPool.Remove(handle);
			}
		}
		virtual void ResizeFrameBuffer(Handle<FrameBuffer> handle, uint32_t width, uint32_t height) override
		{
			if (!handle.IsValid())
			{
				return;
			}

			OpenGLFrameBuffer* frameBuffer = GetFrameBuffer(handle);
			frameBuffer->Resize(width, height);
		}
		OpenGLFrameBuffer* GetFrameBuffer(Handle<FrameBuffer> handle) const
		{
			return m_FrameBufferPool.Get(handle);
		}

		// Shaders
		virtual Handle<Shader> CreateShader(const ShaderDescriptor&& desc) override
		{
			return m_ShaderPool.Insert(std::forward<const ShaderDescriptor>(desc));
		}
		virtual void RecompileShader(Handle<Shader> handle, const ShaderDescriptor&& desc) override
		{
			OpenGLShader* shader = GetShader(handle);
			if (shader != nullptr)
			{
				shader->Destroy();
				shader->Recompile(std::forward<const ShaderDescriptor>(desc));
			}
		}
		virtual void DeleteShader(Handle<Shader> handle) override
		{
			OpenGLShader* shader = GetShader(handle);
			if (shader != nullptr)
			{
				shader->Destroy();
				m_ShaderPool.Remove(handle);
			}
		}
		virtual void AddShaderVariant(Handle<Shader> handle, const ShaderDescriptor::RenderPipeline::Variant& variantDesc) override {}
		OpenGLShader* GetShader(Handle<Shader> handle) const
		{
			return m_ShaderPool.Get(handle);
		}

		// BindGroups
		virtual Handle<BindGroup> CreateBindGroup(const BindGroupDescriptor&& desc) override
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
		virtual void DeleteBindGroup(Handle<BindGroup> handle) override
		{
			OpenGLBindGroup* bindGroup = GetBindGroup(handle);
			if (bindGroup != nullptr)
			{
				bindGroup->Destroy();
				m_BindGroupPool.Remove(handle);
			}
		}
		virtual void UpdateBindGroup(Handle<BindGroup> handle) {}
		virtual uint64_t GetBindGroupHash(Handle<BindGroup> handle) override
		{
			return CalculateBindGroupHash(GetBindGroup(handle));
		}
		OpenGLBindGroup* GetBindGroup(Handle<BindGroup> handle) const
		{
			return m_BindGroupPool.Get(handle);
		}

		// BindGroupsLayouts
		virtual Handle<BindGroupLayout> CreateBindGroupLayout(const BindGroupLayoutDescriptor&& desc) override
		{
			return m_BindGroupLayoutPool.Insert(std::forward<const BindGroupLayoutDescriptor>(desc));
		}
		virtual void DeleteBindGroupLayout(Handle<BindGroupLayout> handle) override
		{
			m_BindGroupLayoutPool.Remove(handle);
		}
		OpenGLBindGroupLayout* GetBindGroupLayout(Handle<BindGroupLayout> handle) const
		{
			return m_BindGroupLayoutPool.Get(handle);
		}

		// RenderPass
		virtual Handle<RenderPass> CreateRenderPass(const RenderPassDescriptor&& desc) override
		{
			return m_RenderPassPool.Insert(std::forward<const RenderPassDescriptor>(desc));
		}
		virtual void DeleteRenderPass(Handle<RenderPass> handle) override
		{
			m_RenderPassPool.Remove(handle);
		}
		OpenGLRenderPass* GetRenderPass(Handle<RenderPass> handle) const
		{
			return m_RenderPassPool.Get(handle);
		}

		// RenderPassLayouts
		virtual Handle<RenderPassLayout> CreateRenderPassLayout(const RenderPassLayoutDescriptor&& desc) override
		{
			return m_RenderPassLayoutPool.Insert(std::forward<const RenderPassLayoutDescriptor>(desc));
		}
		virtual void DeleteRenderPassLayout(Handle<RenderPassLayout> handle) override
		{
			m_RenderPassLayoutPool.Remove(handle);
		}
		OpenGLRenderPassLayout* GetRenderPassLayout(Handle<RenderPassLayout> handle) const
		{
			return m_RenderPassLayoutPool.Get(handle);
		}

	private:
		Pool<OpenGLTexture, Texture> m_TexturePool = Pool<OpenGLTexture, Texture>(128);
		Pool<OpenGLBuffer, Buffer> m_BufferPool = Pool<OpenGLBuffer, Buffer>(512);
		Pool<OpenGLShader, Shader> m_ShaderPool = Pool<OpenGLShader, Shader>(64);
		Pool<OpenGLFrameBuffer, FrameBuffer> m_FrameBufferPool = Pool<OpenGLFrameBuffer, FrameBuffer>(32);
		Pool<OpenGLBindGroup, BindGroup> m_BindGroupPool = Pool<OpenGLBindGroup, BindGroup>(64);
		Pool<OpenGLBindGroupLayout, BindGroupLayout> m_BindGroupLayoutPool = Pool<OpenGLBindGroupLayout, BindGroupLayout>(32);
		Pool<OpenGLRenderPass, RenderPass> m_RenderPassPool = Pool<OpenGLRenderPass, RenderPass>(32);
		Pool<OpenGLRenderPassLayout, RenderPassLayout> m_RenderPassLayoutPool = Pool<OpenGLRenderPassLayout, RenderPassLayout>(32);

	private:
		uint64_t CalculateBindGroupHash(const OpenGLBindGroup* bindGroup)
		{
			if (bindGroup == nullptr)
			{
				return 0;
			}

			uint64_t hash = 0;

			for (const auto& bufferEntry : bindGroup->Buffers)
			{
				hash += bufferEntry.buffer.HashKey();
				hash += bufferEntry.byteOffset;
				hash += bufferEntry.range;
			}

			for (const auto texture : bindGroup->Textures)
			{
				hash += texture.HashKey();
			}

			hash += bindGroup->BindGroupLayout.HashKey();

			return hash;
		}
	};
}
