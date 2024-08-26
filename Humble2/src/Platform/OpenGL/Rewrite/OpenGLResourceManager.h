#pragma once

#include "Renderer\Rewrite\ResourceManager.h"

#include "Renderer\Rewrite\Pool.h"
#include "Renderer\Rewrite\Types.h"
#include "Renderer\Rewrite\TypeDescriptors.h"

#include "OpenGLBuffer.h"
#include "OpenGLShader.h"
#include "OpenGLTexture.h"
#include "OpenGLFrameBuffer.h"
#include "OpenGLBindGroup.h"
#include "OpenGLBindGroupLayout.h"
#include "OpenGLRenderPass.h"
#include "OpenGLRenderPassLayout.h"

namespace HBL2
{
	class OpenGLResourceManager final : public ResourceManager
	{
	public:
		virtual ~OpenGLResourceManager() = default;

		// Textures
		virtual Handle<Texture> CreateTexture(const TextureDescriptor&& desc) override
		{
			return m_TexturePool.Insert(OpenGLTexture(desc));
		}
		virtual void DeleteTexture(Handle<Texture> handle) override
		{
			m_TexturePool.Remove(handle);
		}
		OpenGLTexture* GetTexture(Handle<Texture> handle) const
		{
			return m_TexturePool.Get(handle);
		}

		// Buffers
		virtual Handle<Buffer> CreateBuffer(const BufferDescriptor&& desc) override
		{
			return m_BufferPool.Insert(OpenGLBuffer(desc));
		}
		virtual void DeleteBuffer(Handle<Buffer> handle) override
		{
			m_BufferPool.Remove(handle);
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
		OpenGLBuffer* GetBuffer(Handle<Buffer> handle) const
		{
			return m_BufferPool.Get(handle);
		}

		// Framebuffers
		virtual Handle<FrameBuffer> CreateFrameBuffer(const FrameBufferDescriptor&& desc) override
		{
			return m_FrameBufferPool.Insert(OpenGLFrameBuffer(desc));
		}
		virtual void DeleteFrameBuffer(Handle<FrameBuffer> handle) override
		{
			m_FrameBufferPool.Remove(handle);
		}
		OpenGLFrameBuffer* GetFrameBuffer(Handle<FrameBuffer> handle) const
		{
			return m_FrameBufferPool.Get(handle);
		}

		// Shaders
		virtual Handle<Shader> CreateShader(const ShaderDescriptor&& desc) override
		{
			return m_ShaderPool.Insert(OpenGLShader(desc));
		}
		virtual void DeleteShader(Handle<Shader> handle) override
		{
			m_ShaderPool.Remove(handle);
		}
		OpenGLShader* GetShader(Handle<Shader> handle) const
		{
			return m_ShaderPool.Get(handle);
		}

		// BindGroups
		virtual Handle<BindGroup> CreateBindGroup(const BindGroupDescriptor&& desc) override
		{
			return m_BindGroupPool.Insert(OpenGLBindGroup(desc));
		}
		virtual void DeleteBindGroup(Handle<BindGroup> handle) override
		{
			m_BindGroupPool.Remove(handle);
		}
		OpenGLBindGroup* GetBindGroup(Handle<BindGroup> handle) const
		{
			return m_BindGroupPool.Get(handle);
		}

		// BindGroupsLayouts
		virtual Handle<BindGroupLayout> CreateBindGroupLayout(const BindGroupLayoutDescriptor&& desc) override
		{
			return m_BindGroupLayoutPool.Insert(OpenGLBindGroupLayout(desc));
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
			return m_RenderPassPool.Insert(OpenGLRenderPass(desc));
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
			return m_RenderPassLayoutPool.Insert(OpenGLRenderPassLayout(desc));
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
