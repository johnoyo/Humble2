#include "FastGltfLoader.h"

#include <Project\Project.h>

#include <Asset\AssetManager.h>
#include <Resources\ResourceManager.h>

#include <Utilities\ShaderUtilities.h>
#include <Utilities\TextureUtilities.h>

#include <Utilities\FileDialogs.h>

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

		LoadTextures(path, asset.get());
		LoadMaterials(path, asset.get());

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

	void FastGltfLoader::LoadTextures(const std::filesystem::path& path, const fastgltf::Asset& asset)
	{
		m_Textures.clear();

		size_t numTextures = asset.images.size();
		m_Textures.resize(numTextures);

		for (uint32_t imageIndex = 0; imageIndex < numTextures; ++imageIndex)
		{
			const fastgltf::Image& glTFImage = asset.images[imageIndex];

			Handle<Asset> textureAssetHandle;

			if (auto* filePath = std::get_if<fastgltf::sources::URI>(&glTFImage.data))
			{
				const auto& texturePath = std::filesystem::path(filePath->uri.string());

				if (std::filesystem::exists(texturePath))
				{
					HBL2_CORE_INFO("FastGltfLoader::LoadTexture::AlbedoMap located at: \"{}\".", texturePath);

					const auto& relativeTexturePath = FileUtils::RelativePath(texturePath, Project::GetAssetDirectory());

					if (!std::filesystem::exists(Project::GetAssetFileSystemPath(relativeTexturePath)))
					{
						UUID textureAssetUUID = std::hash<std::string>()(relativeTexturePath);
						textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);

						if (!AssetManager::Instance->IsAssetValid(textureAssetHandle))
						{
							textureAssetHandle = AssetManager::Instance->CreateAsset({
								.debugName = "texture-asset",
								.filePath = relativeTexturePath,
								.type = AssetType::Texture,
							});
						}

						if (textureAssetHandle.IsValid())
						{
							TextureUtilities::Get().CreateAssetMetadataFile(textureAssetHandle);
						}

						AssetManager::Instance->GetAsset<Texture>(textureAssetHandle);
					}
					else
					{
						UUID textureAssetUUID = std::hash<std::string>()(relativeTexturePath);
						textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);
						AssetManager::Instance->GetAsset<Texture>(textureAssetHandle);
					}
				}
				else
				{
					HBL2_CORE_ERROR("Failed to retrieve texture file of \"{}\" gltf file. The texture path {} does not exist!", path, texturePath);
					continue;
				}
			}
			else if (auto* byteArray = std::get_if<fastgltf::sources::Array>(&glTFImage.data))
			{
				const auto& relativeTexturePath = std::filesystem::path("AutoImported") / path.filename().stem() / "Textures" / (std::string(glTFImage.name) + std::to_string(imageIndex) + ".png");

				if (!std::filesystem::exists(Project::GetAssetFileSystemPath(relativeTexturePath)))
				{
					if (!TextureUtilities::Get().Save(Project::GetAssetFileSystemPath(relativeTexturePath), { byteArray->bytes.data(), byteArray->bytes.size() }, true))
					{
						HBL2_CORE_ERROR("Failed to serialize texture file located inside of \"{}\" gltf file.", path);
						continue;
					}

					UUID textureAssetUUID = std::hash<std::string>()(relativeTexturePath.string());
					textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);

					if (!AssetManager::Instance->IsAssetValid(textureAssetHandle))
					{
						textureAssetHandle = AssetManager::Instance->CreateAsset({
							.debugName = "texture-asset",
							.filePath = relativeTexturePath,
							.type = AssetType::Texture,
						});
					}

					if (textureAssetHandle.IsValid())
					{
						TextureUtilities::Get().CreateAssetMetadataFile(textureAssetHandle);
					}

					AssetManager::Instance->GetAsset<Texture>(textureAssetHandle);
				}
				else
				{
					UUID textureAssetUUID = std::hash<std::string>()(relativeTexturePath.string());
					textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);
					AssetManager::Instance->GetAsset<Texture>(textureAssetHandle);
				}
			}
			else if (auto* view = std::get_if<fastgltf::sources::BufferView>(&glTFImage.data))
			{
				auto& bufferView = asset.bufferViews[view->bufferViewIndex];
				auto& bufferFromBufferView = asset.buffers[bufferView.bufferIndex];

				std::visit(
					fastgltf::visitor{
						[&](const fastgltf::sources::Array& arr)
						{
							auto full = std::span<const std::byte>(arr.bytes.data(), arr.bytes.size());

							size_t begin = static_cast<size_t>(bufferView.byteOffset);
							size_t len = static_cast<size_t>(bufferView.byteLength);

							HBL2_CORE_ASSERT(begin + len <= full.size(), "BufferView out of range");

							auto slice = full.subspan(begin, len);

							const auto& relativeTexturePath = std::filesystem::path("AutoImported") / path.filename().stem() / "Textures" / (std::string(glTFImage.name) + std::to_string(imageIndex) + ".png");

							if (!std::filesystem::exists(Project::GetAssetFileSystemPath(relativeTexturePath)))
							{
								if (!TextureUtilities::Get().Save(Project::GetAssetFileSystemPath(relativeTexturePath), { slice.data(), slice.size() }))
								{
									HBL2_CORE_ERROR("Failed to serialize texture file located inside of \"{}\" gltf file.", path);
									return;
								}

								UUID textureAssetUUID = std::hash<std::string>()(relativeTexturePath.string());
								textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);

								if (!AssetManager::Instance->IsAssetValid(textureAssetHandle))
								{
									textureAssetHandle = AssetManager::Instance->CreateAsset({
										.debugName = "texture-asset",
										.filePath = relativeTexturePath,
										.type = AssetType::Texture,
									});
								}

								if (textureAssetHandle.IsValid())
								{
									TextureUtilities::Get().CreateAssetMetadataFile(textureAssetHandle);
								}

								AssetManager::Instance->GetAsset<Texture>(textureAssetHandle);
							}
							else
							{
								UUID textureAssetUUID = std::hash<std::string>()(relativeTexturePath.string());
								textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);
								AssetManager::Instance->GetAsset<Texture>(textureAssetHandle);
							}
						},
						[&](fastgltf::sources::Array& arr) // load from memory
						{
							HBL2_CORE_FATAL("not supported default branch (image data = BUfferView) {}", glTFImage.name);
						},
						[&](auto& arg) // default branch if image data is not supported
						{
							HBL2_CORE_FATAL("not supported default branch (image data = BUfferView) {}", glTFImage.name);
						}
					},
					bufferFromBufferView.data);
			}

			m_Textures[imageIndex] = textureAssetHandle;
		}
	}

	void FastGltfLoader::LoadMaterials(const std::filesystem::path& path, const fastgltf::Asset& asset)
	{
		m_MaterialNameToHandle.clear();

		size_t numMaterials = asset.materials.size();

		for (uint32_t materialIndex = 0; materialIndex < numMaterials; ++materialIndex)
		{
			const fastgltf::Material& glTFMaterial = asset.materials[materialIndex];

			Handle<Asset> materialAssetHandle;
			Handle<Material> materialHandle;

			const auto& relativePath = std::filesystem::path("AutoImported") / path.filename().stem() / "Materials" / (std::string(glTFMaterial.name) + std::to_string(materialIndex) + ".mat");

			if (!std::filesystem::exists(Project::GetAssetFileSystemPath(relativePath)))
			{
				materialAssetHandle = AssetManager::Instance->CreateAsset({
					.debugName = "material-asset",
					.filePath = relativePath,
					.type = AssetType::Material,
				});

				// diffuse color aka base color factor used as constant color, if no diffuse texture is provided
				glm::vec4 albedoColor = glm::make_vec4(glTFMaterial.pbrData.baseColorFactor.data());
				float roughness = glTFMaterial.pbrData.roughnessFactor;
				float metalicness = glTFMaterial.pbrData.metallicFactor;

				Handle<Asset> albedoMapAssetHandle;
				Handle<Asset> normalMapAssetHandle;
				Handle<Asset> metallicRoughnessMapAssetHandle;

				// diffuse map aka basecolor aka albedo
				if (glTFMaterial.pbrData.baseColorTexture.has_value())
				{
					uint32_t diffuseMapIndex = glTFMaterial.pbrData.baseColorTexture.value().textureIndex;
					uint32_t imageIndex = asset.textures[diffuseMapIndex].imageIndex.value();

					albedoMapAssetHandle = m_Textures[imageIndex];
				}

				// normal map
				if (glTFMaterial.normalTexture.has_value())
				{
					uint32_t normalMapIndex = glTFMaterial.normalTexture.value().textureIndex;
					uint32_t imageIndex = asset.textures[normalMapIndex].imageIndex.value();

					normalMapAssetHandle = m_Textures[imageIndex];
				}

				// texture for roughness and metallicness
				if (glTFMaterial.pbrData.metallicRoughnessTexture.has_value())
				{
					int metallicRoughnessMapIndex = glTFMaterial.pbrData.metallicRoughnessTexture.value().textureIndex;
					uint32_t imageIndex = asset.textures[metallicRoughnessMapIndex].imageIndex.value();

					metallicRoughnessMapAssetHandle = m_Textures[imageIndex];
				}

				AssetManager::Instance->WaitForAsyncJobs();

				ShaderUtilities::Get().CreateMaterialAssetFile(materialAssetHandle, {
					.ShaderAssetHandle = {}, // Use built-in shaders depending on material type.
					.AlbedoColor = albedoColor,
					.Glossiness = (float)roughness,
					.AlbedoMapAssetHandle = albedoMapAssetHandle,
					.NormalMapAssetHandle = normalMapAssetHandle,
					.RoughnessMapAssetHandle = metallicRoughnessMapAssetHandle,
					.MetallicMapAssetHandle = metallicRoughnessMapAssetHandle,
				});

				if (materialAssetHandle.IsValid())
				{
					uint32_t type = UINT32_MAX;

					if (glTFMaterial.unlit)
					{
						type = 0;
					}
					else if (!glTFMaterial.pbrData.metallicRoughnessTexture.has_value())
					{
						type = 1;
					}
					else
					{
						type = 2;
					}

					ShaderUtilities::Get().CreateMaterialMetadataFile(materialAssetHandle, type);
				}

				materialHandle = AssetManager::Instance->GetAsset<Material>(materialAssetHandle);
			}
			else
			{
				UUID materialAssetUUID = std::hash<std::string>()(relativePath.string());
				materialHandle = AssetManager::Instance->GetAsset<Material>(materialAssetUUID);
			}

			m_MaterialNameToHandle[glTFMaterial.name.c_str()] = materialHandle;
		}
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

		subMeshDescriptor.embededMaterial = m_MaterialNameToHandle[asset.materials[mesh.primitives[subMeshIndex].materialIndex.value_or(0)].name.c_str()];

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
