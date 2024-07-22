#pragma once

#include "Base.h"
#include "TypeDescriptors.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <filesystem>

namespace HBL2
{
	struct Texture;
	struct Buffer;
	struct Shader;
	struct FrameBuffer;
	struct BindGroup;
	struct BindGroupLayout;
	struct RenderPass;
	struct RenderPassLayout;

	struct Mesh
	{
		Mesh() = default;
		Mesh(const MeshDescriptor& desc)
		{
			DebugName = desc.debugName;
			IndexOffset = desc.indexOffset;
			IndexCount = desc.indexCount;
			VertexOffset = desc.vertexOffset;
			VertexCount = desc.vertexCount;
			IndexBuffer = desc.indexBuffer;
			VertexBuffers = desc.vertexBuffers;
		}

		const char* DebugName = "";
		uint32_t IndexOffset;
		uint32_t IndexCount;
		uint32_t VertexOffset;
		uint32_t VertexCount;
		Handle<Buffer> IndexBuffer;
		std::vector<Handle<Buffer>> VertexBuffers;
	};

	struct Material
	{
		Material() = default;
		Material(const MaterialDescriptor& desc)
		{
			DebugName = desc.debugName;
			Shader = desc.shader;
			BindGroup = desc.bindGroup;
		}

		const char* DebugName = "";
		Handle<Shader> Shader;
		Handle<BindGroup> BindGroup;
	};

	struct Asset
	{
		Asset() = default;
		Asset(const AssetDescriptor& desc)
		{
			DebugName = desc.debugName;
		}

		const char* DebugName = "";
		UUID UUID;
		std::filesystem::path FilePath;
		uint32_t FileFormatVersion = 1;
		void* UserData = nullptr;
		bool Loaded = false;
	};
}
