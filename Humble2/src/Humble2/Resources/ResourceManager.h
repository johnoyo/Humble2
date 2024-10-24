#pragma once

#include "Handle.h"
#include "Pool.h"
#include "Types.h"
#include "Scene\Scene.h"
#include "TypeDescriptors.h"

namespace HBL2
{
	class ResourceManager
	{
	public:
		static inline ResourceManager* Instance;

		virtual ~ResourceManager() = default;

		virtual void Clean() = 0;

		// Textures
		virtual Handle<Texture> CreateTexture(const TextureDescriptor&& desc) = 0;
		virtual void DeleteTexture(Handle<Texture> handle) = 0;

		// Buffers
		virtual Handle<Buffer> CreateBuffer(const BufferDescriptor&& desc) = 0;
		virtual void DeleteBuffer(Handle<Buffer> handle) = 0;
		virtual void ReAllocateBuffer(Handle<Buffer> handle, uint32_t currentOffset) = 0;
		virtual void* GetBufferData(Handle<Buffer> handle) = 0;

		// FrameBuffers
		virtual Handle<FrameBuffer> CreateFrameBuffer(const FrameBufferDescriptor&& desc) = 0;
		virtual void DeleteFrameBuffer(Handle<FrameBuffer> handle) = 0;

		// Shaders
		virtual Handle<Shader> CreateShader(const ShaderDescriptor&& desc) = 0;
		virtual void DeleteShader(Handle<Shader> handle) = 0;

		// BindGroups
		virtual Handle<BindGroup> CreateBindGroup(const BindGroupDescriptor&& desc) = 0;
		virtual void DeleteBindGroup(Handle<BindGroup> handle) = 0;

		// BindGroupLayouts
		virtual Handle<BindGroupLayout> CreateBindGroupLayout(const BindGroupLayoutDescriptor&& desc) = 0;
		virtual void DeleteBindGroupLayout(Handle<BindGroupLayout> handle) = 0;

		// RenderPass
		virtual Handle<RenderPass> CreateRenderPass(const RenderPassDescriptor&& desc) = 0;
		virtual void DeleteRenderPass(Handle<RenderPass> handle) = 0;

		// RenderPassLayouts
		virtual Handle<RenderPassLayout> CreateRenderPassLayout(const RenderPassLayoutDescriptor&& desc) = 0;
		virtual void DeleteRenderPassLayout(Handle<RenderPassLayout> handle) = 0;

		// Meshes	
		Handle<Mesh> CreateMesh(const MeshDescriptor&& desc)
		{
			return m_MeshPool.Insert(Mesh(std::forward<const MeshDescriptor>(desc)));
		}
		void DeleteMesh(Handle<Mesh> handle)
		{
			m_MeshPool.Remove(handle);
		}
		Mesh* GetMesh(Handle<Mesh> handle) const
		{
			return m_MeshPool.Get(handle);
		}

		// Materials
		Handle<Material> CreateMaterial(const MaterialDescriptor&& desc)
		{
			return m_MaterialPool.Insert(Material(std::forward<const MaterialDescriptor>(desc)));
		}
		void DeleteMaterial(Handle<Material> handle)
		{
			m_MaterialPool.Remove(handle);
		}
		Material* GetMaterial(Handle<Material> handle) const
		{
			return m_MaterialPool.Get(handle);
		}
		
		// Scenes
		Handle<Scene> CreateScene(const SceneDescriptor&& desc)
		{
			return m_ScenePool.Insert(Scene(std::forward<const SceneDescriptor>(desc)));
		}
		void DeleteScene(Handle<Scene> handle)
		{
			m_ScenePool.Remove(handle);
		}
		Scene* GetScene(Handle<Scene> handle) const
		{
			return m_ScenePool.Get(handle);
		}

	private:
		Pool<Mesh, Mesh> m_MeshPool;
		Pool<Material, Material> m_MaterialPool;
		Pool<Scene, Scene> m_ScenePool;
	};
}
