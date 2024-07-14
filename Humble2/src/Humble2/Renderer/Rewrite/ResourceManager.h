#pragma once

#include "Handle.h"
#include "TypeDescriptors.h"

namespace HBL
{
	class Mesh;
	class Material;
	class Texture;
	class Buffer;
	class Shader;
	class BindGroup;
	class BindGroupLayout;
	class RenderPass;
	class RenderPassLayout;

	class ResourceManager
	{
	public:
		static inline ResourceManager* Instance;

		virtual ~ResourceManager() = default;

		// Textures
		virtual Handle<Texture> CreateTexture(TextureDescriptor&& desc) = 0;
		virtual void DeleteTexture(Handle<Texture> handle) = 0;

		// Buffers
		virtual Handle<Buffer> CreateBuffer(BufferDescriptor&& desc) = 0;
		virtual void DeleteBuffer(Handle<Buffer> handle) = 0;
		virtual void* GetBufferData(Handle<Buffer> handle) = 0;

		// Shaders
		virtual Handle<Shader> CreateShader(ShaderDescriptor&& desc) = 0;
		virtual void DeleteShader(Handle<Shader> handle) = 0;

		// BindGroups
		virtual Handle<BindGroup> CreateBindGroup(BindGroupDescriptor&& desc) = 0;
		virtual void DeleteBindGroup(Handle<BindGroup> handle) = 0;

		// BindGroupLayouts
		virtual Handle<BindGroupLayout> CreateBindGroupLayout(BindGroupLayoutDescriptor&& desc) = 0;
		virtual void DeleteBindGroupLayout(Handle<BindGroupLayout> handle) = 0;

		// RenderPass
		virtual Handle<RenderPass> CreateRenderPass(RenderPassDescriptor&& desc) = 0;
		virtual void DeleteRenderPass(Handle<RenderPass> handle) = 0;

		// RenderPassLayouts
		virtual Handle<RenderPassLayout> CreateRenderPassLayout(RenderPassLayoutDescriptor&& desc) = 0;
		virtual void DeleteRenderPassLayout(Handle<RenderPassLayout> handle) = 0;

		// Meshes
		virtual Handle<Mesh> CreateMesh(MeshDescriptor&& mesh) = 0;
		virtual void DeleteMesh(Handle<Mesh> handle) = 0;

		// Materials
		virtual Handle<Material> CreateMaterial(MaterialDescriptor&& desc) = 0;
		virtual void DeleteMaterial(Handle<Material> handle) = 0;
	};
}
