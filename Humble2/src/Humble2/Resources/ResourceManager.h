#pragma once

#include "Handle.h"
#include "Pool.h"
#include "Types.h"
#include "Scene\Scene.h"
#include "Scene\Script.h"
#include "TypeDescriptors.h"
#include "ResourceDeletionQueue.h"
#include "Sound\Sound.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
	class HBL2_API ResourceManager
	{
	public:
		static ResourceManager* Instance;

		virtual ~ResourceManager() = default;

		void Flush(uint32_t currentFrame)
		{
			m_DeletionQueue.Flush(currentFrame);
		}

		virtual void Clean() = 0;

		// Textures
		virtual Handle<Texture> CreateTexture(const TextureDescriptor&& desc) = 0;
		virtual void DeleteTexture(Handle<Texture> handle) = 0;
		virtual void UpdateTexture(Handle<Texture> handle, const Span<const std::byte>& bytes) = 0;
		virtual void TransitionTextureLayout(Handle<Texture> handle, TextureLayout currentLayout, TextureLayout newLayout, PipelineStage srcStage, PipelineStage dstStage) = 0;

		// Buffers
		virtual Handle<Buffer> CreateBuffer(const BufferDescriptor&& desc) = 0;
		virtual void DeleteBuffer(Handle<Buffer> handle) = 0;
		virtual void ReAllocateBuffer(Handle<Buffer> handle, uint32_t currentOffset) = 0;
		virtual void* GetBufferData(Handle<Buffer> handle) = 0;
		virtual void SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData) = 0;
		virtual void SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData) = 0;

		// FrameBuffers
		virtual Handle<FrameBuffer> CreateFrameBuffer(const FrameBufferDescriptor&& desc) = 0;
		virtual void DeleteFrameBuffer(Handle<FrameBuffer> handle) = 0;
		virtual void ResizeFrameBuffer(Handle<FrameBuffer> handle, uint32_t width, uint32_t height) = 0;

		// Shaders
		virtual Handle<Shader> CreateShader(const ShaderDescriptor&& desc) = 0;
		virtual void DeleteShader(Handle<Shader> handle) = 0;
		virtual void AddShaderVariant(Handle<Shader> handle, const ShaderDescriptor::RenderPipeline::Variant& variantDesc) = 0;
		uint32_t GetShaderVariantHash(Handle<Shader> handle, const ShaderDescriptor::RenderPipeline::Variant& variantDesc)
		{
			return handle.HashKey() + std::hash<ShaderDescriptor::RenderPipeline::Variant>()(variantDesc);
		}

		// BindGroups
		virtual Handle<BindGroup> CreateBindGroup(const BindGroupDescriptor&& desc) = 0;
		virtual void DeleteBindGroup(Handle<BindGroup> handle) = 0;
		virtual uint64_t GetBindGroupHash(Handle<BindGroup> handle) = 0;
		uint64_t GetBindGroupHash(const BindGroupDescriptor& desc)
		{
			uint64_t hash = 0;

			for (const auto& bufferEntry : desc.buffers)
			{
				hash += bufferEntry.buffer.HashKey();
				hash += bufferEntry.byteOffset;
				hash += bufferEntry.range;
			}

			for (const auto texture : desc.textures)
			{
				hash += texture.HashKey();
			}

			hash += desc.layout.HashKey();

			return hash;
		}

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

		// Scripts
		Handle<Script> CreateScript(const ScriptDescriptor&& desc)
		{
			return m_ScriptPool.Insert(Script(std::forward<const ScriptDescriptor>(desc)));
		}
		void DeleteScript(Handle<Script> handle)
		{
			m_ScriptPool.Remove(handle);
		}
		Script* GetScript(Handle<Script> handle) const
		{
			return m_ScriptPool.Get(handle);
		}

		// Sounds
		Handle<Sound> CreateSound(const SoundDescriptor&& desc)
		{
			return m_SoundPool.Insert(Sound(std::forward<const SoundDescriptor>(desc)));
		}
		void DeleteSound(Handle<Sound> handle)
		{
			m_SoundPool.Remove(handle);
		}
		Sound* GetSound(Handle<Sound> handle) const
		{
			return m_SoundPool.Get(handle);
		}

	protected:
		ResourceDeletionQueue m_DeletionQueue;

	private:
		Pool<Mesh, Mesh> m_MeshPool;
		Pool<Material, Material> m_MaterialPool;
		Pool<Scene, Scene> m_ScenePool;
		Pool<Script, Script> m_ScriptPool;
		Pool<Sound, Sound> m_SoundPool;
	};
}
