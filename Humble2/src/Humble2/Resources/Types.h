#pragma once

#include "Base.h"
#include "TypeDescriptors.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <filesystem>

namespace HBL2
{
	struct Texture {};
	struct Buffer;
	struct Shader {};
	struct FrameBuffer;
	struct BindGroup;
	struct BindGroupLayout;
	struct RenderPass;
	struct RenderPassLayout;

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 UV;
		//glm::vec3 Tangent;
	};

	struct MeshExtents
	{
		glm::vec3 Min = { (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)(), (std::numeric_limits<float>::max)() };
		glm::vec3 Max = { (std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)(), (std::numeric_limits<float>::min)() };
	};

	struct SubMesh
	{
		SubMesh() = default;
		SubMesh(const SubMeshDescriptor&& desc)
		{
			DebugName = desc.debugName;
			IndexOffset = desc.indexOffset;
			IndexCount = desc.indexCount;
			VertexOffset = desc.vertexOffset;
			VertexCount = desc.vertexCount;
			Extents = { desc.minVertex, desc.maxVertex };

			EmbededMaterial = desc.embededMaterial;
		}

		const char* DebugName = "";
		uint32_t IndexOffset = 0;
		uint32_t IndexCount = 0;
		uint32_t VertexOffset = 0;
		uint32_t VertexCount = 0;
		uint32_t InstanceOffset = 0;
		uint32_t InstanceCount = 1;
		MeshExtents Extents;

		Handle<Material> EmbededMaterial;
	};

	struct MeshPart
	{
		MeshPart() = default;
		MeshPart(const MeshPartDescriptor&& desc)
		{
			DebugName = desc.debugName;
			IndexBuffer = desc.indexBuffer;
			VertexBuffers = desc.vertexBuffers;

			for (const SubMeshDescriptor& subMeshDescriptor : desc.subMeshes)
			{
				SubMeshes.emplace_back(std::forward<const SubMeshDescriptor>(subMeshDescriptor));
			}

			for (const SubMesh& subMesh : SubMeshes)
			{
				Extents.Min.x = glm::min(subMesh.Extents.Min.x, Extents.Min.x);
				Extents.Min.y = glm::min(subMesh.Extents.Min.y, Extents.Min.y);
				Extents.Min.z = glm::min(subMesh.Extents.Min.z, Extents.Min.z);

				Extents.Max.x = glm::max(subMesh.Extents.Max.x, Extents.Max.x);
				Extents.Max.y = glm::max(subMesh.Extents.Max.y, Extents.Max.y);
				Extents.Max.z = glm::max(subMesh.Extents.Max.z, Extents.Max.z);
			}

			ImportedLocalTransform = std::move(*((LocalTransform*)&desc.importedLocalTransform));
		}

		const char* DebugName = "";
		std::vector<SubMesh> SubMeshes;
		Handle<Buffer> IndexBuffer;
		std::vector<Handle<Buffer>> VertexBuffers;
		MeshExtents Extents;

		struct LocalTransform
		{
			glm::vec3 translation;
			glm::vec3 rotation;
			glm::vec3 scale;
		} ImportedLocalTransform;
	};

	struct Mesh
	{
		Mesh() = default;
		Mesh(const MeshDescriptor&& desc)
		{
			DebugName = desc.debugName;

			for (const MeshPartDescriptor& meshPartDescriptor : desc.meshes)
			{
				Meshes.emplace_back(std::forward<const MeshPartDescriptor>(meshPartDescriptor));
			}

			for (const MeshPart& meshPart : Meshes)
			{
				Extents.Min.x = glm::min(meshPart.Extents.Min.x, Extents.Min.x);
				Extents.Min.y = glm::min(meshPart.Extents.Min.y, Extents.Min.y);
				Extents.Min.z = glm::min(meshPart.Extents.Min.z, Extents.Min.z);

				Extents.Max.x = glm::max(meshPart.Extents.Max.x, Extents.Max.x);
				Extents.Max.y = glm::max(meshPart.Extents.Max.y, Extents.Max.y);
				Extents.Max.z = glm::max(meshPart.Extents.Max.z, Extents.Max.z);
			}
		}

		const char* DebugName = "";
		std::vector<MeshPart> Meshes;
		MeshExtents Extents;
	};

	struct Material
	{
		Material() = default;
		Material(const MaterialDescriptor&& desc)
		{
			DebugName = desc.debugName;
			Shader = desc.shader;
			BindGroup = desc.bindGroup;
		}

		enum class BlendMode
		{
			Opaque = 0,
			Transparent = 1,
		};

		const char* DebugName = "";
		Handle<Shader> Shader;
		Handle<BindGroup> BindGroup;
		BlendMode BlendMethod = BlendMode::Opaque;

		glm::vec4 AlbedoColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		float Glossiness = 3.0f;
	};
}
