#include "Types.h"

#include "ResourceManager.h"
#include "Renderer\Renderer.h"

namespace HBL2
{
	Mesh::Mesh(const MeshDescriptor&& desc)
	{
		DebugName = desc.debugName;

		uint32_t* indeces = (uint32_t*)desc.indeces.Data();
		uint32_t indecesCount = uint32_t(desc.indeces.Size());

		float* vertices = (float*)desc.vertices.Data();
		uint32_t verticesCount = uint32_t(desc.vertices.Size());

		//Renderer::Instance->Submit([this, vertices, verticesCount, indeces, indecesCount]()
		//{
			Handle<Buffer> indexBufferHandle = ResourceManager::Instance->CreateBuffer({
				.debugName = "terrain-index-buffer",
				.usage = BufferUsage::INDEX,
				.byteSize = (uint32_t)sizeof(uint32_t) * indecesCount,
				.initialData = indeces,
			});

			Handle<Buffer> vertexBufferHandle = ResourceManager::Instance->CreateBuffer({
				.debugName = "terrain-vertex-buffer",
				.usage = BufferUsage::VERTEX,
				.byteSize = (uint32_t)sizeof(float) * verticesCount * 8, // <- 8 because we have 3 pos, 3 normal, 2 uv in the vb.
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

			// Free cpu side buffer data.
			delete[] vertices;
			delete[] indeces;
		//});
	}

	void Mesh::operator=(const Mesh& other)
	{
		DebugName = other.DebugName;
		Meshes = other.Meshes;
		Extents = other.Extents;
		m_HasItems.store(other.m_HasItems.load(std::memory_order_relaxed), std::memory_order_relaxed);
	}
}
