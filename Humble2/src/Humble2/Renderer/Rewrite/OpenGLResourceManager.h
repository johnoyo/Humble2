#pragma once

#include "Renderer\RenderCommand.h"

#include "ResourceManager.h"

#include "Pool.h"
#include "Types.h"
#include "TypeDescriptors.h"

namespace HBL
{
	class OpenGLResourceManager final : public ResourceManager
	{
	public:
		virtual ~OpenGLResourceManager() = default;

		// Textures
		virtual Handle<Texture> CreateTexture(TextureDescriptor&& desc) override
		{
			return m_TexturePool.Insert(OpenGLTexture(desc));
		}
		virtual void DeleteTexture(Handle<Texture> handle) override
		{
			m_TexturePool.Remove(handle);
		}

		// Buffers
		virtual Handle<Buffer> CreateBuffer(BufferDescriptor&& desc) override
		{
			return m_BufferPool.Insert(OpenGLBuffer(desc));
		}
		virtual void DeleteBuffer(Handle<Buffer> handle) override
		{
			m_BufferPool.Remove(handle);
		}
		virtual void* GetBufferData(Handle<Buffer> handle) override
		{
			return m_BufferPool.Get(handle)->Data;
		}

		// Shaders
		virtual Handle<Shader> CreateShader(ShaderDescriptor&& desc) override
		{
			return m_ShaderPool.Insert(OpenGLShader(desc));
		}
		virtual void DeleteShader(Handle<Shader> handle) override
		{
			m_ShaderPool.Remove(handle);
		}

		// BindGroups
		virtual Handle<BindGroup> CreateBindGroup(BindGroupDescriptor&& desc) override
		{
			return m_BindGroupPool.Insert(OpenGLBindGroup(desc));
		}
		virtual void DeleteBindGroup(Handle<BindGroup> handle) override
		{
			m_BindGroupPool.Remove(handle);
		}

		// BindGroupsLayouts
		virtual Handle<BindGroupLayout> CreateBindGroupLayout(BindGroupLayoutDescriptor&& desc) override
		{
			return m_BindGroupLayoutPool.Insert(OpenGLBindGroupLayout(desc));
		}
		virtual void DeleteBindGroupLayout(Handle<BindGroupLayout> handle) override
		{
			m_BindGroupLayoutPool.Remove(handle);
		}

		// RenderPass
		virtual Handle<RenderPass> CreateRenderPass(RenderPassDescriptor&& desc) override
		{
			return m_RenderPassPool.Insert(OpenGLRenderPass(desc));
		}
		virtual void DeleteRenderPass(Handle<RenderPass> handle) override
		{
			m_RenderPassPool.Remove(handle);
		}

		// RenderPassLayouts
		virtual Handle<RenderPassLayout> CreateRenderPassLayout(RenderPassLayoutDescriptor&& desc) override
		{
			return m_RenderPassLayoutPool.Insert(OpenGLRenderPassLayout(desc));
		}
		virtual void DeleteRenderPassLayout(Handle<RenderPassLayout> handle) override
		{
			m_RenderPassLayoutPool.Remove(handle);
		}

		// Meshes
		virtual Handle<Mesh> CreateMesh(MeshDescriptor&& desc) override
		{
			return m_MeshPool.Insert(OpenGLMesh(desc));
		}
		virtual void DeleteMesh(Handle<Mesh> handle) override
		{
			m_MeshPool.Remove(handle);
		}

		// Materials
		virtual Handle<Material> CreateMaterial(MaterialDescriptor&& desc) override
		{
			return m_MaterialPool.Insert(OpenGLMaterial(desc));
		}
		virtual void DeleteMaterial(Handle<Material> handle) override
		{
			m_MaterialPool.Remove(handle);
		}

		// Getters
		OpenGLMesh* GetMesh(Handle<Mesh> handle) const
		{
			return m_MeshPool.Get(handle);
		}
		OpenGLMaterial* GetMaterial(Handle<Material> handle) const
		{
			return m_MaterialPool.Get(handle);
		}
		OpenGLTexture* GetTexture(Handle<Texture> handle) const
		{
			return m_TexturePool.Get(handle);
		}
		OpenGLBuffer* GetBuffer(Handle<Buffer> handle) const
		{
			return m_BufferPool.Get(handle);
		}
		OpenGLShader* GetShader(Handle<Shader> handle) const
		{
			return m_ShaderPool.Get(handle);
		}
		OpenGLBindGroup* GetBindGroup(Handle<BindGroup> handle) const
		{
			return m_BindGroupPool.Get(handle);
		}
		OpenGLBindGroupLayout* GetBindGroupLayout(Handle<BindGroupLayout> handle) const
		{
			return m_BindGroupLayoutPool.Get(handle);
		}
		OpenGLRenderPass* GetRenderPass(Handle<RenderPass> handle) const
		{
			return m_RenderPassPool.Get(handle);
		}
		OpenGLRenderPassLayout* GetRenderPassLayout(Handle<RenderPassLayout> handle) const
		{
			return m_RenderPassLayoutPool.Get(handle);
		}

	private:
		Pool<OpenGLMesh, Mesh> m_MeshPool;
		Pool<OpenGLMaterial, Material> m_MaterialPool;
		Pool<OpenGLTexture, Texture> m_TexturePool;
		Pool<OpenGLBuffer, Buffer> m_BufferPool;
		Pool<OpenGLShader, Shader> m_ShaderPool;
		Pool<OpenGLBindGroup, BindGroup> m_BindGroupPool;
		Pool<OpenGLBindGroupLayout, BindGroupLayout> m_BindGroupLayoutPool;
		Pool<OpenGLRenderPass, RenderPass> m_RenderPassPool;
		Pool<OpenGLRenderPassLayout, RenderPassLayout> m_RenderPassLayoutPool;
	};
}
