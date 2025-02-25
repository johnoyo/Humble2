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
			return m_TexturePool.Insert(OpenGLTexture(std::forward<const TextureDescriptor>(desc)));
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
		OpenGLTexture* GetTexture(Handle<Texture> handle) const
		{
			return m_TexturePool.Get(handle);
		}

		// Buffers
		virtual Handle<Buffer> CreateBuffer(const BufferDescriptor&& desc) override
		{
			return m_BufferPool.Insert(OpenGLBuffer(std::forward<const BufferDescriptor>(desc)));
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
		OpenGLBuffer* GetBuffer(Handle<Buffer> handle) const
		{
			return m_BufferPool.Get(handle);
		}

		// Framebuffers
		virtual Handle<FrameBuffer> CreateFrameBuffer(const FrameBufferDescriptor&& desc) override
		{
			return m_FrameBufferPool.Insert(OpenGLFrameBuffer(std::forward<const FrameBufferDescriptor>(desc)));
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
			return m_ShaderPool.Insert(OpenGLShader(std::forward<const ShaderDescriptor>(desc)));
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
		OpenGLShader* GetShader(Handle<Shader> handle) const
		{
			return m_ShaderPool.Get(handle);
		}

		// BindGroups
		virtual Handle<BindGroup> CreateBindGroup(const BindGroupDescriptor&& desc) override
		{
			return m_BindGroupPool.Insert(OpenGLBindGroup(std::forward<const BindGroupDescriptor>(desc)));
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
		OpenGLBindGroup* GetBindGroup(Handle<BindGroup> handle) const
		{
			return m_BindGroupPool.Get(handle);
		}

		// BindGroupsLayouts
		virtual Handle<BindGroupLayout> CreateBindGroupLayout(const BindGroupLayoutDescriptor&& desc) override
		{
			return m_BindGroupLayoutPool.Insert(OpenGLBindGroupLayout(std::forward<const BindGroupLayoutDescriptor>(desc)));
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
			return m_RenderPassPool.Insert(OpenGLRenderPass(std::forward<const RenderPassDescriptor>(desc)));
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
			return m_RenderPassLayoutPool.Insert(OpenGLRenderPassLayout(std::forward<const RenderPassLayoutDescriptor>(desc)));
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
		Pool<OpenGLTexture, Texture> m_TexturePool;
		Pool<OpenGLBuffer, Buffer> m_BufferPool;
		Pool<OpenGLShader, Shader> m_ShaderPool;
		Pool<OpenGLFrameBuffer, FrameBuffer> m_FrameBufferPool;
		Pool<OpenGLBindGroup, BindGroup> m_BindGroupPool;
		Pool<OpenGLBindGroupLayout, BindGroupLayout> m_BindGroupLayoutPool;
		Pool<OpenGLRenderPass, RenderPass> m_RenderPassPool;
		Pool<OpenGLRenderPassLayout, RenderPassLayout> m_RenderPassLayoutPool;
	};
}
