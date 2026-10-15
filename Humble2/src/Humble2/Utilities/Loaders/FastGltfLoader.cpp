#include "FastGltfLoader.h"

#include "Project/Project.h"

#include "Asset/EditorAssetManager.h"
#include "Resources/ResourceManager.h"

#include "Platform/PlatformManager.h"

#include "Utilities/MeshUtilities.h"
#include "Utilities/ShaderUtilities.h"
#include "Utilities/FileDialogs.h"

#include <stb_image/stb_image_write.h>

#include <numeric>
#include <algorithm>
#include <unordered_map>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace HBL2
{
	thread_local std::vector<Vertex> FastGltfLoader::s_Vertices;
	thread_local std::vector<uint32_t> FastGltfLoader::s_Indices;
	thread_local std::vector<SkinVertex> FastGltfLoader::s_SkinVertices;
	thread_local std::unordered_map<const char*, Handle<Asset>> FastGltfLoader::s_MaterialNameToAssetHandle;

	static bool SplitMetalRoughImage(uint32_t imageIndex, const std::filesystem::path& autoImportedPath, Handle<Asset> metallicRoughnessMapAssetHandle, Handle<Asset>& roughnessMapAssetHandle, Handle<Asset>& metallicMapAssetHandle, bool reload)
	{
		auto* editorAssetManager = (EditorAssetManager*)AssetManager::Instance;

		Asset* metallicRoughnessMapAsset = AssetManager::Instance->GetAssetMetadata(metallicRoughnessMapAssetHandle);

		const auto& metallicRoughnessFileSystemPath = Project::GetAssetFileSystemPath(metallicRoughnessMapAsset->FilePath);
		const auto& relativeRoughPath = autoImportedPath / "Textures" / (std::to_string(imageIndex) + "_rough.png");
		const auto& relativeMetalPath = autoImportedPath / "Textures" / (std::to_string(imageIndex) + "_metal.png");

		UUID roughTextureAssetUUID = editorAssetManager->GetUUIDFromPath(relativeRoughPath);
		roughnessMapAssetHandle = AssetManager::Instance->GetHandleFromUUID(roughTextureAssetUUID);

		UUID metalTextureAssetUUID = editorAssetManager->GetUUIDFromPath(relativeMetalPath);
		metallicMapAssetHandle = AssetManager::Instance->GetHandleFromUUID(metalTextureAssetUUID);

		if (AssetManager::Instance->IsAssetValid(roughnessMapAssetHandle) && AssetManager::Instance->IsAssetValid(metallicMapAssetHandle) && !reload)
		{
			AssetManager::Instance->DeleteAsset(metallicRoughnessMapAssetHandle);
			return true;
		}

		if (reload)
		{
			AssetManager::Instance->DeleteAsset(roughnessMapAssetHandle);
			AssetManager::Instance->DeleteAsset(metallicMapAssetHandle);
		}		

		int w = 0, h = 0, c = 0;
		stbi_uc* src = stbi_load(metallicRoughnessFileSystemPath.string().c_str(), &w, &h, &c, STBI_rgb_alpha);
		if (!src)
		{
			return false;
		}

		std::vector<stbi_uc> rough((size_t)w * h * 4);
		std::vector<stbi_uc> metal((size_t)w * h * 4);

		for (size_t i = 0; i < (size_t)w * h; ++i)
		{
			const stbi_uc g = src[i * 4 + 1]; // roughness
			const stbi_uc b = src[i * 4 + 2]; // metallic

			// Replicate into RGB so .r works and the image previews correctly.
			rough[i * 4 + 0] = g; rough[i * 4 + 1] = g; rough[i * 4 + 2] = g; rough[i * 4 + 3] = 255;
			metal[i * 4 + 0] = b; metal[i * 4 + 1] = b; metal[i * 4 + 2] = b; metal[i * 4 + 3] = 255;
		}

		stbi_image_free(src);

		const auto& autoImportedTexturesPath = Project::GetAssetFileSystemPath(autoImportedPath) / "Textures";

		// Ensure auto imported textures path exists.
		if (!std::filesystem::exists(autoImportedTexturesPath))
		{
			try
			{
				std::filesystem::create_directories(autoImportedTexturesPath);
			}
			catch (std::exception& e)
			{
				HBL2_ERROR("Auto imported textures directory creation failed: {0}", e.what());
			}
		}

		TextureSettings settings;
		settings.Flip = false;

		settings.PlatformCompressionMethod[(int)Platform::Windows] = CompressionMethod::BASISU;
		settings.PlatformCompressionMethod[(int)Platform::MacOS] = CompressionMethod::BASISU;
		settings.PlatformCompressionMethod[(int)Platform::Linux] = CompressionMethod::BASISU;

		settings.PlatformCompressionFormat[(int)Platform::Windows] = Format::BC4_R_UNORM;
		settings.PlatformCompressionFormat[(int)Platform::MacOS] = Format::BC4_R_UNORM;
		settings.PlatformCompressionFormat[(int)Platform::Linux] = Format::BC4_R_UNORM;

		settings.PlatformCompressionQuality[(int)Platform::Windows] = CompressionQuality::MEDIUM;
		settings.PlatformCompressionQuality[(int)Platform::MacOS] = CompressionQuality::MEDIUM;
		settings.PlatformCompressionQuality[(int)Platform::Linux] = CompressionQuality::MEDIUM;

		int roughOk = 1;

		if (!AssetManager::Instance->IsAssetValid(roughnessMapAssetHandle))
		{
			const auto& absoluteRoughPath = autoImportedTexturesPath / (std::to_string(imageIndex) + "_rough.png");

			roughOk = stbi_write_png(absoluteRoughPath.string().c_str(), w, h, 4, rough.data(), w * 4);

			if (roughOk)
			{
				roughnessMapAssetHandle = editorAssetManager->CreateAsset({
					.debugName = "texture-asset",
					.filePath = relativeRoughPath,
					.type = AssetType::Texture,
				});

				if (roughnessMapAssetHandle.IsValid())
				{
					TextureUtilities::Get().SerializeAssetMetadataFile(roughnessMapAssetHandle, settings);
				}

				AssetManager::Instance->GetAsset<Texture>(roughnessMapAssetHandle);
			}
			else
			{
				HBL2_CORE_ERROR("Failed to write image data to {}.", absoluteRoughPath);
			}
		}

		int metalOk = 1;

		if (!AssetManager::Instance->IsAssetValid(metallicMapAssetHandle))
		{
			const auto& absoluteMetalPath = autoImportedTexturesPath / (std::to_string(imageIndex) + "_metal.png");

			metalOk = stbi_write_png(absoluteMetalPath.string().c_str(), w, h, 4, metal.data(), w * 4);

			if (metalOk)
			{
				metallicMapAssetHandle = editorAssetManager->CreateAsset({
					.debugName = "texture-asset",
					.filePath = relativeMetalPath,
					.type = AssetType::Texture,
				});

				if (metallicMapAssetHandle.IsValid())
				{
					TextureUtilities::Get().SerializeAssetMetadataFile(metallicMapAssetHandle, settings);
				}

				AssetManager::Instance->GetAsset<Texture>(metallicMapAssetHandle);
			}
			else
			{
				HBL2_CORE_ERROR("Failed to write image data to {}.", absoluteMetalPath);
			}
		}

		if (roughOk && metalOk)
		{
			AssetManager::Instance->DeleteAsset(metallicRoughnessMapAssetHandle);
		}

		return roughOk && metalOk;
	}

	static glm::mat4 NodeLocalMatrix(const fastgltf::Node& node)
	{
		if (const auto* trs = std::get_if<fastgltf::TRS>(&node.transform))
		{
			const glm::vec3 t(trs->translation.x(), trs->translation.y(), trs->translation.z());
			const glm::quat r(trs->rotation.w(), trs->rotation.x(), trs->rotation.y(), trs->rotation.z());
			const glm::vec3 s(trs->scale.x(), trs->scale.y(), trs->scale.z());
			return glm::translate(glm::mat4(1.0f), t) * glm::mat4_cast(r) * glm::scale(glm::mat4(1.0f), s);
		}

		glm::mat4 m(1.0f);
		const auto& fm = std::get<fastgltf::math::fmat4x4>(node.transform);
		static_assert(sizeof(glm::mat4) == sizeof(float) * 16);
		std::memcpy(&m, fm.data(), sizeof(glm::mat4));
		return m;
	}

	static JointTransform DecomposeTRS(const glm::mat4& m)
	{
		JointTransform out;
		out.T = glm::vec3(m[3]);

		glm::vec3 s(glm::length(glm::vec3(m[0])), glm::length(glm::vec3(m[1])), glm::length(glm::vec3(m[2])));
		if (glm::determinant(glm::mat3(m)) < 0.0f)
		{
			s.x = -s.x; // mirrored transform
		}

		const glm::vec3 inv(s.x != 0.f ? 1.f / s.x : 0.f, s.y != 0.f ? 1.f / s.y : 0.f, s.z != 0.f ? 1.f / s.z : 0.f);
		const glm::mat3 rot(glm::vec3(m[0]) * inv.x, glm::vec3(m[1]) * inv.y, glm::vec3(m[2]) * inv.z);

		out.R = glm::normalize(glm::quat_cast(rot));
		out.S = s;
		return out;
	}

	static bool IsNearIdentity(const glm::mat4& m, float eps = 1e-4f)
	{
		for (int c = 0; c < 4; ++c)
		{
			for (int r = 0; r < 4; ++r)
			{
				if (std::abs(m[c][r] - (c == r ? 1.f : 0.f)) > eps)
				{
					return false;
				}
			}
		}

		return true;
	}

	static uint32_t HashJointName(std::string_view s)
	{
		uint32_t h = 2166136261u;
		for (char c : s) { h ^= (uint8_t)c; h *= 16777619u; }
		return h;
	}

	Handle<Mesh> FastGltfLoader::Load(const std::filesystem::path& path)
	{
		HBL2_FUNC_PROFILE();

		// glTF files list their required extensions
		constexpr auto extensions =
			fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::KHR_materials_emissive_strength |
			fastgltf::Extensions::KHR_lights_punctual | fastgltf::Extensions::KHR_texture_transform;

		constexpr auto gltfOptions =
            fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
			fastgltf::Options::LoadExternalBuffers | fastgltf::Options::GenerateMeshIndices;

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

		LoadMaterials(path, asset.get());

		SkeletonImport skeletonImport = BuildSkeleton(path, asset.get());

		std::vector<MeshPartDescriptor> meshes;
		uint32_t meshIndex = 0;

		for (const auto& node : asset->nodes)
		{
			HBL2_CORE_TRACE("Object: {0}", node.name);

			if (node.meshIndex)
			{
				auto result = LoadMeshData(asset.get(), node, meshIndex++, skeletonImport);
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

		Handle<Skeleton> skeleton;
		std::vector<Handle<AnimationClip>> animationClips;
		CreateSkeletonAndClips(asset.get(), skeletonImport, skeleton, animationClips);

		return ResourceManager::Instance->CreateMesh({
			.debugName = strdup(path.filename().stem().string().c_str()),
			.meshes = std::move(meshes),
			.skeleton = skeleton,
			.animationClips = std::move(animationClips),
		});
	}

	void FastGltfLoader::Reload(Asset* asset)
	{
		HBL2_FUNC_PROFILE();

		// glTF files list their required extensions
		constexpr auto extensions =
			fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::KHR_materials_emissive_strength |
			fastgltf::Extensions::KHR_lights_punctual | fastgltf::Extensions::KHR_texture_transform;

		constexpr auto gltfOptions =
            fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble |
            fastgltf::Options::LoadExternalBuffers | fastgltf::Options::GenerateMeshIndices;

		// Initialize fastgltf parser
		fastgltf::GltfDataBuffer dataBuffer;
		fastgltf::Parser parser(extensions);

		const auto& fileSystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
		const std::filesystem::path& meshPath = std::filesystem::exists(fileSystemPath) ? fileSystemPath : asset->FilePath;

		auto dataBufferResult = dataBuffer.FromPath(meshPath.string());

		if (!dataBufferResult)
		{
			HBL2_CORE_ERROR("Failed to load GLTF file: {0}", meshPath.string());
			return;
		}

		auto gltfAsset = parser.loadGltf(dataBufferResult.get(), meshPath.parent_path(), gltfOptions);
		if (!gltfAsset)
		{
			HBL2_CORE_ERROR("Failed to parse GLTF file: {0}", meshPath.string());
			return;
		}

		ReloadMaterials(meshPath, gltfAsset.get(), asset);

		SkeletonImport skeletonImport = BuildSkeleton(meshPath, gltfAsset.get());

		std::vector<MeshPartDescriptor> meshes;
		uint32_t meshIndex = 0;

		for (const auto& node : gltfAsset->nodes)
		{
			HBL2_CORE_TRACE("Object: {0}", node.name);

			if (node.meshIndex)
			{
				auto result = LoadMeshData(gltfAsset.get(), node, meshIndex++, skeletonImport);
				if (result.IsOk())
				{
					meshes.push_back(result.Unwrap());
				}
			}
		}

		if (meshes.empty())
		{
			return;
		}

		Handle<Mesh> handle = Handle<Mesh>::UnPack(asset->Indentifier);
		Mesh* mesh = ResourceManager::Instance->GetMesh(handle);

		if (mesh == nullptr)
		{
			return;
		}

		mesh->MarkAsEmpty();

		std::vector<Handle<Buffer>> previousBuffers;

		// Collect previous buffers.
		for (const MeshPart& meshPart : mesh->Meshes)
		{
			previousBuffers.push_back(meshPart.SkinBuffer);
			previousBuffers.push_back(meshPart.IndexBuffer);

			for (Handle<Buffer> vb : meshPart.VertexBuffers)
			{
				previousBuffers.push_back(vb);
			}
		}

		// Collect previous skeleton and animation clips.
		const Handle<Skeleton> previousSkeleton = mesh->Skeleton;
		const std::vector<Handle<AnimationClip>> previousClips = mesh->AnimationClips;

		Handle<Skeleton> newSkeleton;
		std::vector<Handle<AnimationClip>> newClips;
		CreateSkeletonAndClips(gltfAsset.get(), skeletonImport, newSkeleton, newClips);

		ResourceManager::Instance->ReimportMesh(handle, {
			.debugName = strdup(meshPath.filename().stem().string().c_str()),
			.meshes = std::move(meshes),
			.skeleton = newSkeleton,
			.animationClips = std::move(newClips),
		});

		// Delete previous buffers.
		for (Handle<Buffer> buffer : previousBuffers)
		{
			ResourceManager::Instance->DeleteBuffer(buffer);
		}

		// Delete previous skeleton and animation clips.
		if (previousSkeleton.IsValid())
		{
			ResourceManager::Instance->DeleteSkeleton(previousSkeleton);
		}
		for (Handle<AnimationClip> clip : previousClips)
		{
			ResourceManager::Instance->DeleteAnimationClip(clip);
		}
	}

	Handle<Asset> FastGltfLoader::LoadTexture(const std::filesystem::path& path, const fastgltf::Asset& asset, uint32_t imageIndex, JobContext& ctx)
	{
		auto* editorAssetManager = (EditorAssetManager*)AssetManager::Instance;

		size_t numTextures = asset.images.size();

		if (imageIndex >= numTextures)
		{
			return {};
		}

		const fastgltf::Image& glTFImage = asset.images[imageIndex];

		Handle<Asset> textureAssetHandle;

		if (auto* filePath = std::get_if<fastgltf::sources::URI>(&glTFImage.data))
		{
			const auto texturePath = path.parent_path() / filePath->uri.fspath();

			if (std::filesystem::exists(texturePath))
			{
				HBL2_CORE_INFO("FastGltfLoader::LoadTextures located at: \"{}\".", texturePath);

				const auto& relativeTexturePath = FileUtils::RelativePath(texturePath, Project::GetAssetDirectory());

				UUID textureAssetUUID = editorAssetManager->GetUUIDFromPath(relativeTexturePath);
				textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);

				// If the texture asset has not been registered yet, register it now.
				if (!AssetManager::Instance->IsAssetValid(textureAssetHandle))
				{
					textureAssetHandle = editorAssetManager->CreateAsset({
						.debugName = "texture-asset",
						.filePath = relativeTexturePath,
						.type = AssetType::Texture,
					});
				}

				if (textureAssetHandle.IsValid())
				{
					TextureUtilities::Get().CreateAssetMetadataFile(textureAssetHandle);
				}

				AssetManager::Instance->GetAssetAsync<Texture>(textureAssetHandle, &ctx);
			}
			else
			{
				HBL2_CORE_ERROR("Failed to retrieve texture file of \"{}\" gltf file. The texture path {} does not exist!", path, texturePath);
				return {};
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
					return {};
				}

				UUID textureAssetUUID = editorAssetManager->GetUUIDFromPath(relativeTexturePath);
				textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);

				if (!AssetManager::Instance->IsAssetValid(textureAssetHandle))
				{
					textureAssetHandle = editorAssetManager->CreateAsset({
						.debugName = "texture-asset",
						.filePath = relativeTexturePath,
						.type = AssetType::Texture,
					});
				}

				if (textureAssetHandle.IsValid())
				{
					TextureUtilities::Get().CreateAssetMetadataFile(textureAssetHandle);
				}

				AssetManager::Instance->GetAssetAsync<Texture>(textureAssetHandle, &ctx);
			}
			else
			{
				UUID textureAssetUUID = editorAssetManager->GetUUIDFromPath(relativeTexturePath);
				textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);
				AssetManager::Instance->GetAssetAsync<Texture>(textureAssetHandle, &ctx);
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

							UUID textureAssetUUID = editorAssetManager->GetUUIDFromPath(relativeTexturePath);
							textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);

							if (!AssetManager::Instance->IsAssetValid(textureAssetHandle))
							{
								textureAssetHandle = editorAssetManager->CreateAsset({
									.debugName = "texture-asset",
									.filePath = relativeTexturePath,
									.type = AssetType::Texture,
								});
							}

							if (textureAssetHandle.IsValid())
							{
								TextureUtilities::Get().CreateAssetMetadataFile(textureAssetHandle);
							}

							AssetManager::Instance->GetAssetAsync<Texture>(textureAssetHandle, &ctx);
						}
						else
						{
							UUID textureAssetUUID = editorAssetManager->GetUUIDFromPath(relativeTexturePath);
							textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);
							AssetManager::Instance->GetAssetAsync<Texture>(textureAssetHandle, &ctx);
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

		return textureAssetHandle;
	}

	Handle<Asset> FastGltfLoader::ReloadTexture(const std::filesystem::path& path, const fastgltf::Asset& asset, uint32_t imageIndex, JobContext& ctx)
	{
		auto* editorAssetManager = (EditorAssetManager*)AssetManager::Instance;

		size_t numTextures = asset.images.size();

		if (imageIndex >= numTextures)
		{
			return {};
		}

		const fastgltf::Image& glTFImage = asset.images[imageIndex];

		Handle<Asset> textureAssetHandle;

		if (auto* filePath = std::get_if<fastgltf::sources::URI>(&glTFImage.data))
		{
			const auto texturePath = path.parent_path() / filePath->uri.fspath();

			if (std::filesystem::exists(texturePath))
			{
				HBL2_CORE_INFO("FastGltfLoader::LoadTexture::AlbedoMap located at: \"{}\".", texturePath);

				const auto& relativeTexturePath = FileUtils::RelativePath(texturePath, Project::GetAssetDirectory());

				UUID textureAssetUUID = editorAssetManager->GetUUIDFromPath(relativeTexturePath);
				textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);

				// If the texture asset has not been registered yet, register it now.
				if (!AssetManager::Instance->IsAssetValid(textureAssetHandle))
				{
					textureAssetHandle = editorAssetManager->CreateAsset({
						.debugName = "texture-asset",
						.filePath = relativeTexturePath,
						.type = AssetType::Texture,
					});
				}

				if (textureAssetHandle.IsValid())
				{
					TextureUtilities::Get().CreateAssetMetadataFile(textureAssetHandle);
				}

				editorAssetManager->ReloadAssetAsync<Texture>(textureAssetHandle, &ctx);
			}
			else
			{
				HBL2_CORE_ERROR("Failed to retrieve texture file of \"{}\" gltf file. The texture path {} does not exist!", path, texturePath);
				return {};
			}
		}
		else if (auto* byteArray = std::get_if<fastgltf::sources::Array>(&glTFImage.data))
		{
			const auto& relativeTexturePath = std::filesystem::path("AutoImported") / path.filename().stem() / "Textures" / (std::string(glTFImage.name) + std::to_string(imageIndex) + ".png");

			UUID textureAssetUUID = editorAssetManager->GetUUIDFromPath(relativeTexturePath);
			textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);

			// Save the data from the gltf file again to disk, in case they got updated.
			if (!TextureUtilities::Get().Save(Project::GetAssetFileSystemPath(relativeTexturePath), { byteArray->bytes.data(), byteArray->bytes.size() }, true))
			{
				HBL2_CORE_ERROR("Failed to serialize texture file located inside of \"{}\" gltf file.", path);
				return {};
			}

			if (!AssetManager::Instance->IsAssetValid(textureAssetHandle))
			{
				textureAssetHandle = editorAssetManager->CreateAsset({
					.debugName = "texture-asset",
					.filePath = relativeTexturePath,
					.type = AssetType::Texture,
				});
			}

			if (textureAssetHandle.IsValid())
			{
				TextureUtilities::Get().CreateAssetMetadataFile(textureAssetHandle);
			}

			editorAssetManager->ReloadAssetAsync<Texture>(textureAssetHandle, &ctx);
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

						UUID textureAssetUUID = editorAssetManager->GetUUIDFromPath(relativeTexturePath);
						textureAssetHandle = AssetManager::Instance->GetHandleFromUUID(textureAssetUUID);

						// Save the data from the gltf file again to disk, in case they got updated.
						if (!TextureUtilities::Get().Save(Project::GetAssetFileSystemPath(relativeTexturePath), { slice.data(), slice.size() }))
						{
							HBL2_CORE_ERROR("Failed to serialize texture file located inside of \"{}\" gltf file.", path);
							return;
						}

						if (!AssetManager::Instance->IsAssetValid(textureAssetHandle))
						{
							textureAssetHandle = editorAssetManager->CreateAsset({
								.debugName = "texture-asset",
								.filePath = relativeTexturePath,
								.type = AssetType::Texture,
							});
						}

						if (textureAssetHandle.IsValid())
						{
							TextureUtilities::Get().CreateAssetMetadataFile(textureAssetHandle);
						}

						editorAssetManager->DeleteAssetImmediate(textureAssetHandle);
						editorAssetManager->GetAssetAsync<Texture>(textureAssetHandle, &ctx);
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

		return textureAssetHandle;
	}

	void FastGltfLoader::LoadMaterials(const std::filesystem::path& path, const fastgltf::Asset& asset)
	{
		auto* editorAssetManager = (EditorAssetManager*)AssetManager::Instance;

		s_MaterialNameToAssetHandle.clear();

		size_t numMaterials = asset.materials.size();

		for (uint32_t materialIndex = 0; materialIndex < numMaterials; ++materialIndex)
		{
			const fastgltf::Material& glTFMaterial = asset.materials[materialIndex];

			Handle<Asset> materialAssetHandle;
			Handle<Material> materialHandle;

			const auto& relativeBasePath = std::filesystem::path("AutoImported") / path.filename().stem();
			const auto& relativePath = relativeBasePath / "Materials" / (std::string(glTFMaterial.name) + std::to_string(materialIndex) + ".mat");

			if (!std::filesystem::exists(Project::GetAssetFileSystemPath(relativePath)))
			{
				materialAssetHandle = editorAssetManager->CreateAsset({
					.debugName = "material-asset",
					.filePath = relativePath,
					.type = AssetType::Material,
				});

				// diffuse color aka base color factor used as constant color, if no diffuse texture is provided
				glm::vec4 albedoColor = glm::make_vec4(glTFMaterial.pbrData.baseColorFactor.data());
				float roughness = glTFMaterial.pbrData.roughnessFactor;
				float metallicness = glTFMaterial.pbrData.metallicFactor;

				Handle<Asset> albedoMapAssetHandle;
				Handle<Asset> normalMapAssetHandle;
				Handle<Asset> metallicRoughnessMapAssetHandle;

				Handle<Asset> roughnessMapAssetHandle;
				Handle<Asset> metallicMapAssetHandle;

				JobContext ctx;

				// diffuse map aka basecolor aka albedo
				if (glTFMaterial.pbrData.baseColorTexture.has_value())
				{
					uint32_t diffuseMapIndex = (uint32_t)glTFMaterial.pbrData.baseColorTexture.value().textureIndex;
					uint32_t imageIndex = (uint32_t)asset.textures[diffuseMapIndex].imageIndex.value();

					albedoMapAssetHandle = LoadTexture(path, asset, imageIndex, ctx);
				}

				// normal map
				if (glTFMaterial.normalTexture.has_value())
				{
					uint32_t normalMapIndex = (uint32_t)glTFMaterial.normalTexture.value().textureIndex;
					uint32_t imageIndex = (uint32_t)asset.textures[normalMapIndex].imageIndex.value();

					normalMapAssetHandle = LoadTexture(path, asset, imageIndex, ctx);
				}

				// texture for roughness and metallicness
				if (glTFMaterial.pbrData.metallicRoughnessTexture.has_value())
				{
                    uint32_t metallicRoughnessMapIndex = (uint32_t)glTFMaterial.pbrData.metallicRoughnessTexture.value().textureIndex;
					uint32_t imageIndex = (uint32_t)asset.textures[metallicRoughnessMapIndex].imageIndex.value();
										
					metallicRoughnessMapAssetHandle = LoadTexture(path, asset, imageIndex, ctx);
				}

				AssetManager::Instance->WaitForAsyncJobs(&ctx);

				bool isPBR = (normalMapAssetHandle.IsValid() && metallicRoughnessMapAssetHandle.IsValid());

				if (isPBR)
				{
					uint32_t metallicRoughnessMapIndex = (uint32_t)glTFMaterial.pbrData.metallicRoughnessTexture.value().textureIndex;
					uint32_t imageIndex = (uint32_t)asset.textures[metallicRoughnessMapIndex].imageIndex.value();

					SplitMetalRoughImage(imageIndex, relativeBasePath, metallicRoughnessMapAssetHandle, roughnessMapAssetHandle, metallicMapAssetHandle, false);
				}

				// Get material uniform buffer data.
				struct MaterialParams { glm::vec4 Color; float Roughness; float Metallic; };
				MaterialParams params{ albedoColor, (float)roughness, (float)metallicness };

				std::vector<uint8_t> uniformBufferBytes(sizeof(MaterialParams));
				memcpy(uniformBufferBytes.data(), &params, sizeof(params));

				Handle<Asset> shaderAssetHandle;
				ShaderReflectionData shaderReflectionData;

				MeshUtilities::Get().GetShaderAssetHandleAndReflectionData(shaderAssetHandle, shaderReflectionData, isPBR);

				bool blendEnbled = false;

				if (glTFMaterial.alphaMode == fastgltf::AlphaMode::Blend)
				{
					blendEnbled = true;
				}

				ShaderUtilities::Get().CreateMaterialAssetFile(materialAssetHandle, {
					.ShaderAssetHandle = shaderAssetHandle,
					.VariantHash =
					{
						.blendEnabled = blendEnbled,
						.depthWrite = false,
						.depthCompare = (ShaderDescriptor::RenderPipeline::packed_size)Compare::LESS_OR_EQUAL,
					},
					.ReflectionData = &shaderReflectionData,
					.Buffers = { uniformBufferBytes },
					.TextureAssets = { albedoMapAssetHandle.Pack(), normalMapAssetHandle.Pack(), metallicMapAssetHandle.Pack(), roughnessMapAssetHandle.Pack() },
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

					ShaderUtilities::Get().CreateMaterialMetadataFile(materialAssetHandle, type, true);
				}

				materialHandle = AssetManager::Instance->GetAsset<Material>(materialAssetHandle);
			}
			else
			{
                UUID materialAssetUUID = editorAssetManager->GetUUIDFromPath(relativePath);
                materialAssetHandle = editorAssetManager->GetHandleFromUUID(materialAssetUUID);
				materialHandle = AssetManager::Instance->GetAsset<Material>(materialAssetHandle);
			}

			s_MaterialNameToAssetHandle[glTFMaterial.name.c_str()] = materialAssetHandle;
		}
	}

	void FastGltfLoader::ReloadMaterials(const std::filesystem::path& path, const fastgltf::Asset& asset, Asset* sourceAsset)
	{
		auto* editorAssetManager = (EditorAssetManager*)AssetManager::Instance;

		// Load the mesh settings.
		MeshSettings meshSettings = MeshUtilities::Get().DeserializeAssetMetadataFile(sourceAsset);

		s_MaterialNameToAssetHandle.clear();

		size_t numMaterials = asset.materials.size();

		for (uint32_t materialIndex = 0; materialIndex < numMaterials; ++materialIndex)
		{
			const fastgltf::Material& glTFMaterial = asset.materials[materialIndex];

			Handle<Asset> materialAssetHandle;
			Handle<Material> materialHandle;

			const auto& relativeBasePath = std::filesystem::path("AutoImported") / path.filename().stem();
			const auto& relativePath = relativeBasePath / "Materials" / (std::string(glTFMaterial.name) + std::to_string(materialIndex) + ".mat");

			// diffuse color aka base color factor used as constant color, if no diffuse texture is provided
			glm::vec4 albedoColor = glm::make_vec4(glTFMaterial.pbrData.baseColorFactor.data());
			float roughness = glTFMaterial.pbrData.roughnessFactor;
			float metallicness = glTFMaterial.pbrData.metallicFactor;

			Handle<Asset> albedoMapAssetHandle;
			Handle<Asset> normalMapAssetHandle;
			Handle<Asset> metallicRoughnessMapAssetHandle;

			Handle<Asset> roughnessMapAssetHandle;
			Handle<Asset> metallicMapAssetHandle;

			JobContext ctx;

			// diffuse map aka basecolor aka albedo
			if (glTFMaterial.pbrData.baseColorTexture.has_value())
			{
				uint32_t diffuseMapIndex = (uint32_t)glTFMaterial.pbrData.baseColorTexture.value().textureIndex;
				uint32_t imageIndex = (uint32_t)asset.textures[diffuseMapIndex].imageIndex.value();

				if (meshSettings.ReimportTexturesOnReload)
				{
					albedoMapAssetHandle = ReloadTexture(path, asset, imageIndex, ctx);
				}
				else
				{
					albedoMapAssetHandle = LoadTexture(path, asset, imageIndex, ctx);
				}
			}

			// normal map
			if (glTFMaterial.normalTexture.has_value())
			{
				uint32_t normalMapIndex = (uint32_t)glTFMaterial.normalTexture.value().textureIndex;
				uint32_t imageIndex = (uint32_t)asset.textures[normalMapIndex].imageIndex.value();

				if (meshSettings.ReimportTexturesOnReload)
				{
					normalMapAssetHandle = ReloadTexture(path, asset, imageIndex, ctx);
				}
				else
				{
					normalMapAssetHandle = LoadTexture(path, asset, imageIndex, ctx);
				}
			}

			// texture for roughness and metallicness
			if (glTFMaterial.pbrData.metallicRoughnessTexture.has_value())
			{
                uint32_t metallicRoughnessMapIndex = (uint32_t)glTFMaterial.pbrData.metallicRoughnessTexture.value().textureIndex;
				uint32_t imageIndex = (uint32_t)asset.textures[metallicRoughnessMapIndex].imageIndex.value();
				
				if (meshSettings.ReimportTexturesOnReload)
				{
					metallicRoughnessMapAssetHandle = ReloadTexture(path, asset, imageIndex, ctx);
				}
				else
				{
					metallicRoughnessMapAssetHandle = LoadTexture(path, asset, imageIndex, ctx);
				}
			}

			AssetManager::Instance->WaitForAsyncJobs(&ctx);

			bool isPBR = (normalMapAssetHandle.IsValid() && metallicRoughnessMapAssetHandle.IsValid());

			if (isPBR)
			{
				uint32_t metallicRoughnessMapIndex = (uint32_t)glTFMaterial.pbrData.metallicRoughnessTexture.value().textureIndex;
				uint32_t imageIndex = (uint32_t)asset.textures[metallicRoughnessMapIndex].imageIndex.value();

				SplitMetalRoughImage(imageIndex, relativeBasePath, metallicRoughnessMapAssetHandle, roughnessMapAssetHandle, metallicMapAssetHandle, meshSettings.ReimportTexturesOnReload);
			}

			// Get material uniform buffer data.
			struct MaterialParams { glm::vec4 Color; float Roughness; float Metallic; };
			MaterialParams params{ albedoColor, (float)roughness, (float)metallicness };

			std::vector<uint8_t> uniformBufferBytes(sizeof(MaterialParams));
			memcpy(uniformBufferBytes.data(), &params, sizeof(params));

			Handle<Asset> shaderAssetHandle;
			ShaderReflectionData shaderReflectionData;
				
			MeshUtilities::Get().GetShaderAssetHandleAndReflectionData(shaderAssetHandle, shaderReflectionData, isPBR);

			if (!std::filesystem::exists(Project::GetAssetFileSystemPath(relativePath)))
			{
				bool blendEnbled = false;

				if (glTFMaterial.alphaMode == fastgltf::AlphaMode::Blend)
				{
					blendEnbled = true;
				}

				materialAssetHandle = editorAssetManager->CreateAsset({
					.debugName = "material-asset",
					.filePath = relativePath,
					.type = AssetType::Material,
				});

				ShaderUtilities::Get().CreateMaterialAssetFile(materialAssetHandle, {
					.ShaderAssetHandle = shaderAssetHandle,
					.VariantHash =
					{
						.blendEnabled = blendEnbled,
						.depthWrite = false,
						.depthCompare = (ShaderDescriptor::RenderPipeline::packed_size)Compare::LESS_OR_EQUAL,
					},
					.ReflectionData = &shaderReflectionData,
					.Buffers = { uniformBufferBytes },
					.TextureAssets = { albedoMapAssetHandle.Pack(), normalMapAssetHandle.Pack(), metallicMapAssetHandle.Pack(), roughnessMapAssetHandle.Pack() },
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

					ShaderUtilities::Get().CreateMaterialMetadataFile(materialAssetHandle, type, true);
				}

				materialHandle = AssetManager::Instance->GetAsset<Material>(materialAssetHandle);
			}
			else
			{
                UUID materialAssetUUID = editorAssetManager->GetUUIDFromPath(relativePath);
                materialAssetHandle = AssetManager::Instance->GetHandleFromUUID(materialAssetUUID);

				// Recreate material asset file with new data.
				ShaderUtilities::Get().CreateMaterialAssetFile(materialAssetHandle, {
					.ShaderAssetHandle = shaderAssetHandle,
					.VariantHash =
					{
						.blendEnabled = false,
						.depthWrite = false,
						.depthCompare = (ShaderDescriptor::RenderPipeline::packed_size)Compare::LESS_OR_EQUAL,
					},
					.ReflectionData = &shaderReflectionData,
					.Buffers = { uniformBufferBytes },
					.TextureAssets = { albedoMapAssetHandle.Pack(), normalMapAssetHandle.Pack(), metallicMapAssetHandle.Pack(), roughnessMapAssetHandle.Pack() },
				});

				// Reload material now that we have set the new resourses.
				materialHandle = editorAssetManager->ReloadAsset<Material>(materialAssetHandle);
			}

			s_MaterialNameToAssetHandle[glTFMaterial.name.c_str()] = materialAssetHandle;
		}
	}

	SkeletonImport FastGltfLoader::BuildSkeleton(const std::filesystem::path& path, const fastgltf::Asset& asset)
	{
		SkeletonImport out;

		if (asset.skins.empty())
		{
			return out;
		}

		if (asset.skins.size() > 1)
		{
			HBL2_CORE_WARN("{}: file has {} skins, only the first one is imported.", path.filename().string(), asset.skins.size());
		}

		// 1. Pick the skin.
		const fastgltf::Skin& skin = asset.skins[0];
		const size_t numJoints = skin.joints.size();

		if (numJoints == 0 || numJoints > INT16_MAX)
		{
			HBL2_CORE_ERROR("{}: unsupported joint count {}.", path.filename().string(), numJoints);
			return out;
		}

		// glTF only stores children, so build node parents.
		std::vector<int32_t> nodeParent(asset.nodes.size(), -1);
		for (size_t n = 0; n < asset.nodes.size(); ++n)
		{
			for (size_t child : asset.nodes[n].children)
			{
				nodeParent[child] = (int32_t)n;
			}
		}

		// glTF node -> index in skin.joints.
		std::vector<int32_t> nodeToSkinJoint(asset.nodes.size(), -1);
		for (size_t j = 0; j < numJoints; ++j)
		{
			nodeToSkinJoint[skin.joints[j]] = (int32_t)j;
		}

		// 2. Parent joint of each joint, plus the static matrix of any non-joint nodes in between.
		std::vector<int32_t> parentJoint(numJoints, -1);
		std::vector<glm::mat4> ancestorMatrix(numJoints, glm::mat4(1.0f));

		for (size_t j = 0; j < numJoints; ++j)
		{
			glm::mat4 extra(1.0f);
			int32_t n = nodeParent[skin.joints[j]];

			while (n != -1 && nodeToSkinJoint[n] == -1)
			{
				extra = NodeLocalMatrix(asset.nodes[n]) * extra;
				n = nodeParent[n];
			}

			parentJoint[j] = (n == -1) ? -1 : nodeToSkinJoint[n];
			ancestorMatrix[j] = extra;
		}

		// Sort by depth (stable) so that parent index < child index.
		std::vector<uint32_t> depth(numJoints, 0);
		for (size_t j = 0; j < numJoints; ++j)
		{
			uint32_t d = 0;
			for (int32_t p = parentJoint[j]; p != -1; p = parentJoint[p]) { ++d; }
			depth[j] = d;
		}

		std::vector<uint32_t> order(numJoints);
		std::iota(order.begin(), order.end(), 0u);
		std::stable_sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) { return depth[a] < depth[b]; });

		std::vector<uint16_t> oldToNew(numJoints);
		for (uint32_t i = 0; i < numJoints; ++i)
		{
			oldToNew[order[i]] = (uint16_t)i;
		}

		// 4a. Inverse bind matrices, in skin.joints order (identity if the accessor is missing, per spec).
		std::vector<glm::mat4> ibmOld(numJoints, glm::mat4(1.0f));
		if (skin.inverseBindMatrices.has_value())
		{
			const fastgltf::Accessor& accessor = asset.accessors[*skin.inverseBindMatrices];
			size_t k = 0;
			fastgltf::iterateAccessor<fastgltf::math::fmat4x4>(asset, accessor, [&](const fastgltf::math::fmat4x4& m)
			{
				if (k < numJoints)
				{
					std::memcpy(&ibmOld[k], m.data(), sizeof(glm::mat4));
				}
				++k;
			});
		}

		// 3 + 4b. Build the final arrays in sorted order.
		SkeletonDescriptor& d = out.Descriptor;
		d.debugName = strdup(path.filename().stem().string().c_str());
		d.parents.resize(numJoints);
		d.names.resize(numJoints);
		d.nameHashes.resize(numJoints);
		d.inverseBind.resize(numJoints);
		d.bindPose.resize(numJoints);

		bool haveRootTransform = false;

		for (uint32_t i = 0; i < numJoints; ++i)
		{
			const uint32_t old = order[i];
			const fastgltf::Node& node = asset.nodes[skin.joints[old]];

			d.parents[i] = parentJoint[old] == -1 ? int16_t(-1) : (int16_t)oldToNew[parentJoint[old]];

			d.names[i] = node.name.empty() ? ("joint_" + std::to_string(i)) : std::string(node.name);
			d.nameHashes[i] = HashJointName(d.names[i]);
			d.inverseBind[i] = ibmOld[old];

			glm::mat4 local = NodeLocalMatrix(node);

			if (parentJoint[old] == -1)
			{
				// Root joint: keep ancestors as a separate transform so animation can't overwrite them.
				if (!haveRootTransform)
				{
					d.rootTransform = ancestorMatrix[old];
					haveRootTransform = true;
				}
				else if (!IsNearIdentity(glm::inverse(d.rootTransform) * ancestorMatrix[old]))
				{
					HBL2_CORE_WARN("{}: multiple root joints with different ancestor transforms, using the first.", path.filename().string());
				}
			}
			else if (!IsNearIdentity(ancestorMatrix[old]))
			{
				// Non-joint node between two joints: baked into the bind pose, animation will override it.
				HBL2_CORE_WARN("{}: joint '{}' has a non-joint ancestor with a transform, baking it into the bind pose.", path.filename().string(), d.names[i]);
				local = ancestorMatrix[old] * local;
			}

			d.bindPose[i] = DecomposeTRS(local);
		}

		out.NodeToJoint.assign(asset.nodes.size(), -1);
		for (uint32_t old = 0; old < numJoints; ++old)
		{
			out.NodeToJoint[skin.joints[old]] = oldToNew[old];
		}

		out.SkinJointRemap = std::move(oldToNew);
		out.SkinIndex = 0;
		out.Valid = true;

		return out;
	}

	AnimationSampler FastGltfLoader::BuildSampler(const fastgltf::Asset& asset, const fastgltf::AnimationSampler& src)
	{
		AnimationSampler s;

		switch (src.interpolation)
		{
		case fastgltf::AnimationInterpolation::Step:        s.interpolation = AnimationSampler::STEP; break;
		case fastgltf::AnimationInterpolation::CubicSpline: s.interpolation = AnimationSampler::CUBICSPLINE; break;
		default:                                            s.interpolation = AnimationSampler::LINEAR; break;
		}

		const fastgltf::Accessor& inAccessor = asset.accessors[src.inputAccessor];
		s.inputs.reserve(inAccessor.count);
		fastgltf::iterateAccessor<float>(asset, inAccessor, [&](float t) { s.inputs.push_back(t); });

		// Rotations are VEC4, translation/scale are VEC3. iterateAccessor also de-normalizes integer rotation formats.
		const fastgltf::Accessor& outAccessor = asset.accessors[src.outputAccessor];
		std::vector<glm::vec4> raw;
		raw.reserve(outAccessor.count);

		if (outAccessor.type == fastgltf::AccessorType::Vec4)
		{
			fastgltf::iterateAccessor<fastgltf::math::fvec4>(asset, outAccessor, [&](const fastgltf::math::fvec4& v)
			{
				raw.emplace_back(v.x(), v.y(), v.z(), v.w());
			});
		}
		else if (outAccessor.type == fastgltf::AccessorType::Vec3)
		{
			fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset, outAccessor, [&](const fastgltf::math::fvec3& v)
			{
				raw.emplace_back(v.x(), v.y(), v.z(), 0.f);
			});
		}
		else
		{
			return AnimationSampler{}; // empty sampler, SampleClip skips it
		}

		const bool cubic = s.interpolation == AnimationSampler::CUBICSPLINE;
		const size_t keys = s.inputs.size();

		if (raw.size() != (cubic ? keys * 3 : keys))
		{
			HBL2_CORE_ERROR("Animation sampler output count ({}) doesn't match {} keyframes.", raw.size(), keys);
			return AnimationSampler{};
		}

		if (cubic)
		{
			s.in_tangents.resize(keys);
			s.values.resize(keys);
			s.out_tangents.resize(keys);

			for (size_t i = 0; i < keys; ++i)
			{
				s.in_tangents[i] = raw[i * 3 + 0];
				s.values[i] = raw[i * 3 + 1];
				s.out_tangents[i] = raw[i * 3 + 2];
			}
		}
		else
		{
			s.values = std::move(raw);
		}

		return s;
	}

	void FastGltfLoader::BuildAnimationClips(const fastgltf::Asset& asset, const SkeletonImport& skeleton, std::vector<AnimationClipDescriptor>& out)
	{
		if (!skeleton.Valid)
		{
			if (!asset.animations.empty())
			{
				HBL2_CORE_WARN("File has animations but no skin, skipping them (node animation isn't supported).");
			}
			return;
		}

		out.reserve(asset.animations.size());

		for (size_t animIndex = 0; animIndex < asset.animations.size(); ++animIndex)
		{
			const fastgltf::Animation& anim = asset.animations[animIndex];

			std::vector<AnimationSampler> samplers;
			std::vector<AnimationChannel> channels;
			std::unordered_map<size_t, uint32_t> samplerRemap; // glTF sampler index -> index in our array

			uint32_t skipped = 0;

			for (const fastgltf::AnimationChannel& ch : anim.channels)
			{
				if (!ch.nodeIndex.has_value() || ch.path == fastgltf::AnimationPath::Weights)
				{
					++skipped; // morph targets come later
					continue;
				}

				const int32_t joint = skeleton.NodeToJoint[*ch.nodeIndex];
				if (joint < 0)
				{
					++skipped; // animates a node that isn't part of the skeleton
					continue;
				}

				auto [it, inserted] = samplerRemap.try_emplace(ch.samplerIndex, (uint32_t)samplers.size());
				if (inserted)
				{
					samplers.push_back(BuildSampler(asset, anim.samplers[ch.samplerIndex]));
				}

				AnimationChannel c;
				c.JointIndex = (uint16_t)joint;
				c.SamplerIndex = it->second;

				switch (ch.path)
				{
				case fastgltf::AnimationPath::Translation: c.Path = AnimationChannel::TRANSLATION; break;
				case fastgltf::AnimationPath::Rotation:    c.Path = AnimationChannel::ROTATION; break;
				default:                                   c.Path = AnimationChannel::SCALE; break;
				}

				channels.push_back(c);
			}

			if (skipped > 0)
			{
				HBL2_CORE_WARN("Animation '{}': skipped {} channel(s) that don't target skeleton joints (or are morph weights).", std::string(anim.name), skipped);
			}

			// 6. Shift the whole clip so it starts at 0 (shared offset so channels stay in sync).
			float minT = std::numeric_limits<float>::max();
			float maxT = std::numeric_limits<float>::lowest();

			for (const AnimationSampler& s : samplers)
			{
				if (!s.inputs.empty())
				{
					minT = std::min(minT, s.inputs.front());
					maxT = std::max(maxT, s.inputs.back());
				}
			}

			if (channels.empty() || minT > maxT)
			{
				HBL2_CORE_WARN("Animation '{}' has no usable channels, skipping.", std::string(anim.name));
				continue;
			}

			for (AnimationSampler& s : samplers)
			{
				for (float& t : s.inputs) { t -= minT; }
			}

			// Normalize rotation values once per sampler (tangents are left alone).
			std::vector<bool> normalized(samplers.size(), false);
			for (const AnimationChannel& c : channels)
			{
				if (c.Path != AnimationChannel::ROTATION || normalized[c.SamplerIndex]) continue;

				for (glm::vec4& q : samplers[c.SamplerIndex].values)
				{
					const float len = glm::length(q);
					q = len > 1e-6f ? q / len : glm::vec4(0.f, 0.f, 0.f, 1.f);
				}
				normalized[c.SamplerIndex] = true;
			}

			AnimationClipDescriptor desc;
			desc.name = anim.name.empty() ? ("animation_" + std::to_string(animIndex)) : std::string(anim.name);
			desc.samplers = std::move(samplers);
			desc.channels = std::move(channels);
			desc.duration = maxT - minT;
			out.push_back(std::move(desc));
		}
	}

	void FastGltfLoader::CreateSkeletonAndClips(const fastgltf::Asset & asset, SkeletonImport & import, Handle<Skeleton>&skeleton, std::vector<Handle<AnimationClip>>&clips)
	{
		if (!import.Valid)
		{
			return;
		}

		skeleton = ResourceManager::Instance->CreateSkeleton(std::move(import.Descriptor));

		std::vector<AnimationClipDescriptor> clipDescriptors;
		BuildAnimationClips(asset, import, clipDescriptors);

		for (AnimationClipDescriptor& d : clipDescriptors)
		{
			d.skeleton = skeleton;
			clips.push_back(ResourceManager::Instance->CreateAnimationClip(std::move(d)));
		}
	}

	Result<MeshPartDescriptor> FastGltfLoader::LoadMeshData(const fastgltf::Asset& asset, const fastgltf::Node& node, uint32_t meshIndex, const SkeletonImport& skeleton)
	{
		s_Vertices.clear();
		s_Indices.clear();
		s_SkinVertices.clear();

		bool skinned = false;
		if (node.skinIndex.has_value())
		{
			if (skeleton.Valid && *node.skinIndex == skeleton.SkinIndex)
			{
				skinned = true;
			}
			else
			{
				HBL2_CORE_WARN("Node '{}' uses a skin that wasn't imported, rendering it as a static mesh.", std::string(node.name));
			}
		}

		MeshPartDescriptor meshPartDescriptor{};
		const fastgltf::Mesh& mesh = asset.meshes[*node.meshIndex];

		uint32_t numSubMeshes = (uint32_t)mesh.primitives.size();
		if (numSubMeshes)
		{
			meshPartDescriptor.debugName = strdup(node.name.c_str());
			meshPartDescriptor.subMeshes.resize(numSubMeshes);

			for (uint32_t subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
			{
				auto result = LoadSubMeshVertexData(asset, mesh, subMeshIndex, skinned, skeleton);
				if (result.IsOk())
				{
					meshPartDescriptor.subMeshes[subMeshIndex] = std::move(result.Unwrap());
				}
				else
				{
					HBL2_CORE_ERROR(result.GetError());
					continue;
				}
			}

			// Node transform, computed once.
			auto& imported = meshPartDescriptor.importedLocalTransform;

			if (skinned)
			{
				// glTF: the node transform of a skinned mesh is ignored, the joints drive everything.
				imported.translation = { 0.f, 0.f, 0.f };
				imported.rotation = { 0.f, 0.f, 0.f };
				imported.scale = { 1.f, 1.f, 1.f };
			}
			else if (auto* trs = std::get_if<fastgltf::TRS>(&node.transform))
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

		auto vertexBuffer = ResourceManager::Instance->CreateBuffer({
			.debugName = "gltf-mesh-vertex-buffer",
			.usage = (skinned ? BitFlags<BufferUsage>{ BufferUsage::VERTEX, BufferUsage::STORAGE } : BufferUsage::VERTEX),
			.memoryUsage = MemoryUsage::GPU_ONLY,
			.byteSize = static_cast<uint32_t>(s_Vertices.size() * sizeof(Vertex)),
			.initialData = s_Vertices.data(),
		});

		meshPartDescriptor.vertexBuffers = { vertexBuffer };

		if (skinned && !s_SkinVertices.empty())
		{
			meshPartDescriptor.skinBuffer = ResourceManager::Instance->CreateBuffer({
				.debugName = "gltf-mesh-skin-buffer",
				.usage = BufferUsage::STORAGE,
				.memoryUsage = MemoryUsage::GPU_ONLY,
				.byteSize = static_cast<uint32_t>(s_SkinVertices.size() * sizeof(SkinVertex)),
				.initialData = s_SkinVertices.data(),
			});
		}

		if (!s_Indices.empty())
		{
			meshPartDescriptor.indexBuffer = ResourceManager::Instance->CreateBuffer({
				.debugName = "gltf-mesh-index-buffer",
				.usage = BufferUsage::INDEX,
				.memoryUsage = MemoryUsage::GPU_ONLY,
				.byteSize = static_cast<uint32_t>(s_Indices.size() * sizeof(uint32_t)),
				.initialData = s_Indices.data(),
			});
		}

		return meshPartDescriptor;
	}

	Result<SubMeshDescriptor> FastGltfLoader::LoadSubMeshVertexData(const fastgltf::Asset& asset, const fastgltf::Mesh& mesh, uint32_t subMeshIndex, bool skinned, const SkeletonImport& skeleton)
	{
		SubMeshDescriptor subMeshDescriptor{};
		subMeshDescriptor.debugName = strdup(mesh.name.c_str());

		const fastgltf::Primitive& primitive = mesh.primitives[subMeshIndex];

		subMeshDescriptor.embededMaterial = s_MaterialNameToAssetHandle[asset.materials[mesh.primitives[subMeshIndex].materialIndex.value_or(0)].name.c_str()];

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

		//// TANGENT (xyz = tangent, w = handedness)
		//if (auto it = primitive.findAttribute("TANGENT"); it != primitive.attributes.end() && vertexCount)
		//{
		//	hasTangents = true;
		//	fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, asset.accessors[it->accessorIndex],
		//		[&](const fastgltf::math::fvec4& t, size_t i)
		//		{
		//			if (i >= vertexCount) return;
		//			vertices[i].Tangent = { t.x(), t.y(), t.z(), t.w() < 0.f ? -1.f : 1.f };
		//		});
		//}

		// Append data to model's vertex buffer
		uint32_t numVerticesBefore = (uint32_t)s_Vertices.size();
		uint32_t numIndicesBefore = (uint32_t)s_Indices.size();
		s_Vertices.resize(numVerticesBefore + vertexCount);

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

			s_Vertices[vertexIndex++] = vertex;
		}

		if (skinned)
		{
			// Keep the skin stream in lockstep with s_Vertices. Defaults = joint 0, weight 1 (safe fallback).
			s_SkinVertices.resize(numVerticesBefore + vertexCount);

			const auto jointsIt = primitive.findAttribute("JOINTS_0");
			const auto weightsIt = primitive.findAttribute("WEIGHTS_0");

			if (jointsIt != primitive.attributes.end() && weightsIt != primitive.attributes.end())
			{
				const std::vector<uint16_t>& remap = skeleton.SkinJointRemap;

				// 5. Joint indices (ubyte or ushort in the file, converted to u16), remapped to the final joint order.
				fastgltf::iterateAccessorWithIndex<fastgltf::math::u16vec4>(asset, asset.accessors[jointsIt->accessorIndex],
					[&](const fastgltf::math::u16vec4& j, size_t i)
					{
						if (i >= vertexCount) return;
						SkinVertex& sv = s_SkinVertices[numVerticesBefore + i];
						for (int k = 0; k < 4; ++k)
						{
							sv.Joints[k] = j[k] < remap.size() ? remap[j[k]] : uint16_t(0);
						}
					});

				// Weights (float, or normalized ubyte/ushort, converted to float).
				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, asset.accessors[weightsIt->accessorIndex],
					[&](const fastgltf::math::fvec4& w, size_t i)
					{
						if (i >= vertexCount) return;
						SkinVertex& sv = s_SkinVertices[numVerticesBefore + i];
						sv.Weights[0] = w.x(); sv.Weights[1] = w.y(); sv.Weights[2] = w.z(); sv.Weights[3] = w.w();
					});

				// Renormalize (exporters are often slightly off) and zero out joints with no weight.
				for (size_t i = 0; i < vertexCount; ++i)
				{
					SkinVertex& sv = s_SkinVertices[numVerticesBefore + i];
					const float sum = sv.Weights[0] + sv.Weights[1] + sv.Weights[2] + sv.Weights[3];

					if (sum > 1e-6f)
					{
						for (int k = 0; k < 4; ++k) { sv.Weights[k] /= sum; }
					}
					else
					{
						sv.Weights[0] = 1.f; sv.Weights[1] = sv.Weights[2] = sv.Weights[3] = 0.f;
						sv.Joints[0] = 0;
					}

					for (int k = 0; k < 4; ++k)
					{
						if (sv.Weights[k] <= 0.f) { sv.Joints[k] = 0; }
					}
				}

				if (primitive.findAttribute("JOINTS_1") != primitive.attributes.end())
				{
					HBL2_CORE_WARN("Primitive '{}' has more than 4 joint influences per vertex, extra influences are ignored.", std::string(mesh.name));
				}
			}
			else
			{
				HBL2_CORE_WARN("Skinned primitive '{}' is missing JOINTS_0/WEIGHTS_0, binding it to joint 0.", std::string(mesh.name));
			}
		}

		// Indices
		if (primitive.indicesAccessor.has_value())
		{
			auto& accessor = asset.accessors[primitive.indicesAccessor.value()];
			indexCount = accessor.count;

			// append indices for submesh to global index array
			s_Indices.resize(s_Indices.size() + indexCount);
			uint32_t* destination = s_Indices.data() + numIndicesBefore;
			fastgltf::iterateAccessorWithIndex<uint32_t>(asset, accessor, [&](uint32_t subMeshIndex, size_t iterator)
			{
				destination[iterator] = subMeshIndex;
			});
		}

		subMeshDescriptor.vertexCount = (uint32_t)vertexCount;
		subMeshDescriptor.indexCount = (uint32_t)indexCount;

		return subMeshDescriptor;
	}
}
