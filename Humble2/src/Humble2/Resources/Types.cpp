#include "Types.h"

#include "ResourceManager.h"
#include "Renderer\Renderer.h"

namespace HBL2
{
	Mesh::Mesh(const MeshDescriptor&& desc)
	{
		Reimport(std::forward<const MeshDescriptor>(desc));
	}

	Mesh::Mesh(const MeshDescriptorEx&& desc)
	{
		Reimport(std::forward<const MeshDescriptorEx>(desc));
	}

	bool Mesh::IsEmpty()
	{
		return !m_HasItems.load(std::memory_order_acquire);
	}

	void Mesh::MarkAsEmpty()
	{
		m_HasItems.store(false, std::memory_order_release);
	}

	void Mesh::Reimport(const MeshDescriptor&& desc)
	{
		Meshes.clear();
		DebugName = desc.debugName;

		uint32_t* indeces = (uint32_t*)desc.indeces.Data();
		uint32_t indecesCount = uint32_t(desc.indeces.Size());

		float* vertices = (float*)desc.vertices.Data();
		uint32_t verticesCount = uint32_t(desc.vertices.Size());

		Handle<Buffer> indexBufferHandle = ResourceManager::Instance->CreateBuffer({
			.debugName = "terrain-index-buffer",
			.usage = BufferUsage::INDEX,
			.byteSize = (uint32_t)sizeof(uint32_t) * indecesCount,
			.initialData = indeces,
		});

		Handle<Buffer> vertexBufferHandle = ResourceManager::Instance->CreateBuffer({
			.debugName = "terrain-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.byteSize = (uint32_t)sizeof(float) * verticesCount,
			.initialData = vertices,
		});

		Meshes.emplace_back(MeshPartDescriptor{
			.debugName = "mesh-part",
			.subMeshes = {
				{
					.indexCount = indecesCount,
					.vertexOffset = 0,
					.vertexCount = verticesCount,
				}
			},
			.indexBuffer = indexBufferHandle,
			.vertexBuffers = { vertexBufferHandle },
		});

		for (const MeshPart& meshPart : Meshes)
		{
			Extents.Min.x = glm::min(meshPart.Extents.Min.x, Extents.Min.x);
			Extents.Min.y = glm::min(meshPart.Extents.Min.y, Extents.Min.y);
			Extents.Min.z = glm::min(meshPart.Extents.Min.z, Extents.Min.z);

			Extents.Max.x = glm::max(meshPart.Extents.Max.x, Extents.Max.x);
			Extents.Max.y = glm::max(meshPart.Extents.Max.y, Extents.Max.y);
			Extents.Max.z = glm::max(meshPart.Extents.Max.z, Extents.Max.z);
		}

		m_HasItems.store(true, std::memory_order_release);
	}

	void Mesh::Reimport(const MeshDescriptorEx&& desc)
	{
		Meshes.clear();
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

		m_HasItems.store(true, std::memory_order_release);
	}

	Material::Material(const MaterialDescriptor&& desc)
	{
		Reimport(std::forward<const MaterialDescriptor>(desc));
	}

	void Material::Reimport(const MaterialDescriptor&& desc)
	{
		DebugName = desc.debugName;
		Shader = desc.shader;
		BindGroup = desc.bindGroup;
	}
}
