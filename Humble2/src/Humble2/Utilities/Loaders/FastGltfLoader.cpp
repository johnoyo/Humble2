#include "FastGltfLoader.h"

#include <Resources\ResourceManager.h>

namespace HBL2
{
	Handle<Mesh> FastGltfLoader::Load(const std::filesystem::path& path)
	{
		HBL2_FUNC_PROFILE();

		// glTF files list their required extensions
		constexpr auto extensions =
			fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::KHR_materials_emissive_strength |
			fastgltf::Extensions::KHR_lights_punctual | fastgltf::Extensions::KHR_texture_transform;

		constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
			fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers |
			fastgltf::Options::LoadExternalImages | fastgltf::Options::GenerateMeshIndices;

		// Initialize fastgltf parser
		fastgltf::GltfDataBuffer dataBuffer;
		fastgltf::Parser parser(extensions);

		auto dataBufferResult = dataBuffer.FromPath(path.string());

		if (!dataBufferResult)
		{
			HBL2_CORE_ERROR("Failed to load GLTF file: {0}", path.string());
			return Handle<Mesh>();
		}

		auto asset = parser.loadGltf(dataBufferResult.get(), path.parent_path(), gltfOptions);
		if (!asset)
		{
			HBL2_CORE_ERROR("Failed to parse GLTF file: {0}", path.string());
			return Handle<Mesh>();
		}

		std::vector<MeshPartDescriptor> meshes;
		uint32_t meshIndex = 0;

		for (const auto& node : asset->nodes)
		{
			HBL2_CORE_TRACE("Object: {0}", node.name);

			if (node.meshIndex)
			{
				auto result = LoadMeshData(asset.get(), node, meshIndex++);
				if (result.IsOk())
				{
					meshes.push_back(result.Unwrap());
				}
			}
		}

		if (meshes.empty())
		{
			return Handle<Mesh>();
		}

		Handle<Mesh> handle = ResourceManager::Instance->CreateMesh({
			.debugName = _strdup(path.filename().stem().string().c_str()),
			.meshes = std::move(meshes),
		});

		return handle;
	}

	Result<MeshPartDescriptor> FastGltfLoader::LoadMeshData(const fastgltf::Asset& asset, const fastgltf::Node& node, uint32_t meshIndex)
	{
		m_Vertices.clear();
		m_Indices.clear();

		MeshPartDescriptor meshPartDescriptor{};
		const fastgltf::Mesh& mesh = asset.meshes[*node.meshIndex];

		uint32_t numSubMeshes = mesh.primitives.size();
		if (numSubMeshes)
		{
			meshPartDescriptor.debugName = _strdup(node.name.c_str());
			meshPartDescriptor.subMeshes.resize(numSubMeshes);

			for (uint32_t subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
			{
				auto result = LoadSubMeshVertexData(asset, mesh, subMeshIndex);
				if (result.IsOk())
				{
					meshPartDescriptor.subMeshes[subMeshIndex] = std::move(result.Unwrap());
				}
				else
				{
					HBL2_CORE_ERROR(result.GetError());
					continue;
				}

				if (auto* trs = std::get_if<fastgltf::TRS>(&node.transform))
				{
					const fastgltf::math::fvec3& localTr = trs->translation;
					const fastgltf::math::fvec3& localScl = trs->scale;

					// Convert FastGLTF quaternion to GLM quaternion
					glm::quat glmQuat(trs->rotation.w(), trs->rotation.x(), trs->rotation.y(), trs->rotation.z());
					glm::vec3 eulerDegrees = glm::degrees(glm::eulerAngles(glmQuat));

					meshPartDescriptor.importedLocalTransform.translation = { localTr.x(), localTr.y(), localTr.z() };
					meshPartDescriptor.importedLocalTransform.rotation = { eulerDegrees.x, eulerDegrees.y, eulerDegrees.z };
					meshPartDescriptor.importedLocalTransform.scale = { localScl.x(), localScl.y(), localScl.z() };
				}
				else if (auto* mat = std::get_if<fastgltf::math::fmat4x4>(&node.transform))
				{
					fastgltf::math::fvec3 localTr;
					fastgltf::math::fquat localRot;
					fastgltf::math::fvec3 localScl;

					fastgltf::math::decomposeTransformMatrix(*mat, localScl, localRot, localTr);

					// Convert FastGLTF quaternion to GLM quaternion
					glm::quat glmQuat(localRot.w(), localRot.x(), localRot.y(), localRot.z());
					glm::vec3 eulerDegrees = glm::degrees(glm::eulerAngles(glmQuat));

					meshPartDescriptor.importedLocalTransform.translation = { localTr.x(), localTr.y(), localTr.z() };
					meshPartDescriptor.importedLocalTransform.rotation = { eulerDegrees.x, eulerDegrees.y, eulerDegrees.z };
					meshPartDescriptor.importedLocalTransform.scale = { localScl.x(), localScl.y(), localScl.z() };
				}
			}
		}

		auto vertexBuffer = ResourceManager::Instance->CreateBuffer({
			.debugName = "gltf-mesh-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.memoryUsage = MemoryUsage::GPU_ONLY,
			.byteSize = static_cast<uint32_t>(m_Vertices.size() * sizeof(Vertex)),
			.initialData = m_Vertices.data(),
		});

		meshPartDescriptor.vertexBuffers = { vertexBuffer };

		if (!m_Indices.empty())
		{
			meshPartDescriptor.indexBuffer = ResourceManager::Instance->CreateBuffer({
				.debugName = "gltf-mesh-index-buffer",
				.usage = BufferUsage::INDEX,
				.memoryUsage = MemoryUsage::GPU_ONLY,
				.byteSize = static_cast<uint32_t>(m_Indices.size() * sizeof(uint32_t)),
				.initialData = m_Indices.data(),
			});
		}

		return meshPartDescriptor;
	}

	Result<SubMeshDescriptor> FastGltfLoader::LoadSubMeshVertexData(const fastgltf::Asset& asset, const fastgltf::Mesh& mesh, uint32_t subMeshIndex)
	{
		SubMeshDescriptor subMeshDescriptor{};
		subMeshDescriptor.debugName = _strdup(mesh.name.c_str());

		const fastgltf::Primitive& primitive = mesh.primitives[subMeshIndex];

		size_t vertexCount = 0;
		size_t indexCount = 0;

		const float* positionBuffer = nullptr;
		const float* normalsBuffer = nullptr;
		const float* texCoordsBuffer = nullptr;

		// Accessor for positions
		if (auto it = primitive.findAttribute("POSITION"); it != primitive.attributes.end())
		{
			auto& positionAccessor = asset.accessors[it->accessorIndex];
			LoadAccessor<float>(asset, positionAccessor, positionBuffer, &vertexCount);
		}

		// Accessor for normals
		if (auto it = primitive.findAttribute("NORMAL"); it != primitive.attributes.end())
		{
			auto& normalAccessor = asset.accessors[it->accessorIndex];
			LoadAccessor<float>(asset, normalAccessor, normalsBuffer);
		}

		// Accessor for UVs
		if (auto it = primitive.findAttribute("TEXCOORD_0"); it != primitive.attributes.end())
		{
			auto& texCoordsAccessor = asset.accessors[it->accessorIndex];
			LoadAccessor<float>(asset, texCoordsAccessor, texCoordsBuffer);
		}

		// Append data to model's vertex buffer
		uint32_t numVerticesBefore = m_Vertices.size();
		uint32_t numIndicesBefore = m_Indices.size();
		m_Vertices.resize(numVerticesBefore + vertexCount);

		uint32_t vertexIndex = numVerticesBefore;

		subMeshDescriptor.vertexOffset = numVerticesBefore;
		subMeshDescriptor.indexOffset = numIndicesBefore;

		for (size_t vertexIterator = 0; vertexIterator < vertexCount; ++vertexIterator)
		{
			Vertex vertex{};

			// Position
			auto position = positionBuffer ? glm::make_vec3(&positionBuffer[vertexIterator * 3]) : glm::vec3(0.0f);
			vertex.Position = glm::vec3(position.x, position.y, position.z);

			subMeshDescriptor.minVertex.x = glm::min(vertex.Position.x, subMeshDescriptor.minVertex.x);
			subMeshDescriptor.minVertex.y = glm::min(vertex.Position.y, subMeshDescriptor.minVertex.y);
			subMeshDescriptor.minVertex.z = glm::min(vertex.Position.z, subMeshDescriptor.minVertex.z);

			subMeshDescriptor.maxVertex.x = glm::max(vertex.Position.x, subMeshDescriptor.maxVertex.x);
			subMeshDescriptor.maxVertex.y = glm::max(vertex.Position.y, subMeshDescriptor.maxVertex.y);
			subMeshDescriptor.maxVertex.z = glm::max(vertex.Position.z, subMeshDescriptor.maxVertex.z);

			// Normal
			vertex.Normal = glm::normalize( glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[vertexIterator * 3]) : glm::vec3(0.0f)));

			// UV
			vertex.UV = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[vertexIterator * 2]) : glm::vec3(0.0f);

			m_Vertices[vertexIndex++] = vertex;
		}

		// Indices
		if (primitive.indicesAccessor.has_value())
		{
			auto& accessor = asset.accessors[primitive.indicesAccessor.value()];
			indexCount = accessor.count;

			// append indices for submesh to global index array
			m_Indices.resize(m_Indices.size() + indexCount);
			uint32_t* destination = m_Indices.data() + numIndicesBefore;
			fastgltf::iterateAccessorWithIndex<uint32_t>(asset, accessor, [&](uint32_t subMeshIndex, size_t iterator)
			{
				destination[iterator] = subMeshIndex;
			});
		}

		subMeshDescriptor.vertexCount = vertexCount;
		subMeshDescriptor.indexCount = indexCount;

		return subMeshDescriptor;
	}
}
