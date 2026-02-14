#pragma once

#include "Handle.h"
#include "Pool.h"
#include "Types.h"
#include "TypeDescriptors.h"
#include "ResourceDeletionQueue.h"

#include "Scene\Scene.h"
#include "Sound\Sound.h"
#include "Script\Script.h"
#include "Prefab\Prefab.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
	class CommandBuffer;

	struct ResourceManagerSpecification
	{
		uint32_t Textures = 128;
		uint32_t Buffers = 512;
		uint32_t FrameBuffers = 32;
		uint32_t Shaders = 64;
		uint32_t BindGroups = 64;
		uint32_t BindGroupLayouts = 32;
		uint32_t RenderPass = 32;
		uint32_t RenderPassLayouts = 32;
		uint32_t Meshes = 256;
		uint32_t Materials = 64;
		uint32_t Scenes = 16;
		uint32_t Scripts = 32;
		uint32_t Sounds = 32;
		uint32_t Prefabs = 64;
	};

	class HBL2_API ResourceManager
	{
	public:
		static ResourceManager* Instance;

		ResourceManager() = default;
		virtual ~ResourceManager() = default;

		const ResourceManagerSpecification& GetSpec() const;
		void Flush(uint32_t currentFrame);
		void FlushAll();

		virtual void Initialize(const ResourceManagerSpecification& spec) = 0;
		virtual const ResourceManagerSpecification GetUsageStats() = 0;
		virtual void Clean() = 0;

		// Textures
		virtual Handle<Texture> CreateTexture(const TextureDescriptor&& desc) = 0;
		virtual void DeleteTexture(Handle<Texture> handle) = 0;
		virtual void UpdateTexture(Handle<Texture> handle, const Span<const std::byte>& bytes) = 0;
		virtual void TransitionTextureLayout(CommandBuffer* commandBuffer, Handle<Texture> handle, TextureLayout currentLayout, TextureLayout newLayout, Handle<BindGroup> bindGroupHandle) = 0;
		virtual glm::vec3 GetTextureDimensions(Handle<Texture> handle) = 0;
		virtual void* GetTextureData(Handle<Texture> handle) = 0;

		// Buffers
		virtual Handle<Buffer> CreateBuffer(const BufferDescriptor&& desc) = 0;
		virtual void DeleteBuffer(Handle<Buffer> handle) = 0;
		virtual void ReAllocateBuffer(Handle<Buffer> handle, uint32_t currentOffset) = 0;
		virtual void* GetBufferData(Handle<Buffer> handle) = 0;
		virtual void SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData) = 0;
		virtual void SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData) = 0;
		virtual void MapBufferData(Handle<Buffer> buffer, intptr_t offset, intptr_t size) = 0;

		// FrameBuffers
		virtual Handle<FrameBuffer> CreateFrameBuffer(const FrameBufferDescriptor&& desc) = 0;
		virtual void DeleteFrameBuffer(Handle<FrameBuffer> handle) = 0;
		virtual void ResizeFrameBuffer(Handle<FrameBuffer> handle, uint32_t width, uint32_t height) = 0;

		// Shaders
		virtual Handle<Shader> CreateShader(const ShaderDescriptor&& desc) = 0;
		virtual void RecompileShader(Handle<Shader> handle, const ShaderDescriptor&& desc) = 0;
		virtual void DeleteShader(Handle<Shader> handle) = 0;
		virtual uint64_t GetOrAddShaderVariant(Handle<Shader> handle, const ShaderDescriptor::RenderPipeline::PackedVariant& variantDesc) = 0;

		// BindGroups
		virtual Handle<BindGroup> CreateBindGroup(const BindGroupDescriptor&& desc) = 0;
		virtual void DeleteBindGroup(Handle<BindGroup> handle) = 0;
		virtual void UpdateBindGroup(Handle<BindGroup> handle) = 0;
		virtual uint64_t GetBindGroupHash(Handle<BindGroup> handle) = 0;
		uint64_t GetBindGroupHash(const BindGroupDescriptor& desc)
		{
			uint64_t hash = 0;

			for (const auto& bufferEntry : desc.buffers)
			{
				hash += bufferEntry.buffer.HashKey() + typeid(Buffer).hash_code();
				hash += bufferEntry.byteOffset;
				hash += bufferEntry.range;
			}

			for (const auto texture : desc.textures)
			{
				hash += texture.HashKey() + typeid(Texture).hash_code();
			}

			hash += desc.layout.HashKey() + typeid(BindGroupLayout).hash_code();

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
		Handle<Mesh> CreateMesh(const MeshDescriptorEx&& desc);
		Handle<Mesh> CreateMesh(const MeshDescriptor&& desc);
		void ReimportMesh(Handle<Mesh> handle, const MeshDescriptor&& desc);
		void ReimportMesh(Handle<Mesh> handle, const MeshDescriptorEx&& desc);
		void DeleteMesh(Handle<Mesh> handle);
		Mesh* GetMesh(Handle<Mesh> handle) const;

		// Materials
		Handle<Material> CreateMaterial(const MaterialDescriptor&& desc);
		void DeleteMaterial(Handle<Material> handle);
		Material* GetMaterial(Handle<Material> handle) const;
		
		// Scenes
		Handle<Scene> CreateScene(const SceneDescriptor&& desc);
		void DeleteScene(Handle<Scene> handle);
		Scene* GetScene(Handle<Scene> handle) const;

		// Scripts
		Handle<Script> CreateScript(const ScriptDescriptor&& desc);
		void DeleteScript(Handle<Script> handle);
		Script* GetScript(Handle<Script> handle) const;

		// Sounds
		Handle<Sound> CreateSound(const SoundDescriptor&& desc);
		void DeleteSound(Handle<Sound> handle);
		Sound* GetSound(Handle<Sound> handle) const;

		// Prefabs
		Handle<Prefab> CreatePrefab(const PrefabDescriptor&& desc);
		void DeletePrefab(Handle<Prefab> handle);
		Prefab* GetPrefab(Handle<Prefab> handle) const;

	protected:
		void InternalInitialize();

		ResourceDeletionQueue m_DeletionQueue;
		ResourceManagerSpecification m_Spec;

		Pool<Mesh, Mesh> m_MeshPool;
		Pool<Material, Material> m_MaterialPool;
		Pool<Scene, Scene> m_ScenePool;
		Pool<Script, Script> m_ScriptPool;
		Pool<Sound, Sound> m_SoundPool;
		Pool<Prefab, Prefab> m_PrefabPool;
	};
}
