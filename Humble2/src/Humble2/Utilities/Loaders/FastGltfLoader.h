#pragma once

#include "Base.h"
#include "Asset/Asset.h"
#include "Asset/AssetManager.h"
#include "Resources/Types.h"
#include "Utilities/TextureUtilities.h"
#include "Utilities/Result.h"
#include "Utilities/Collections/Span.h"

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>

#include <filesystem>

namespace HBL2
{
	class FastGltfLoader
	{
	public:
		Handle<Mesh> Load(const std::filesystem::path& path);
        void Reload(Asset* asset);

	private:
        Handle<Asset> LoadTexture(const std::filesystem::path& path, const fastgltf::Asset& asset, uint32_t imageIndex, JobContext& ctx);
        Handle<Asset> ReloadTexture(const std::filesystem::path& path, const fastgltf::Asset& asset, uint32_t imageIndex, JobContext& ctx);

        void LoadMaterials(const std::filesystem::path& path, const fastgltf::Asset& asset);
        void ReloadMaterials(const std::filesystem::path& path, const fastgltf::Asset& asset, Asset* sourceAsset);

        void LoadAnimations(const std::filesystem::path& path, const fastgltf::Asset& asset, std::vector<Handle<AnimationClip>>& animationClips);

		Result<MeshPartDescriptor> LoadMeshData(const fastgltf::Asset& asset, const fastgltf::Node& node, uint32_t meshIndex);
		Result<SubMeshDescriptor> LoadSubMeshVertexData(const fastgltf::Asset& asset, const fastgltf::Mesh& mesh, uint32_t subMeshIndex);

        template <typename T>
        void LoadAccessor(const fastgltf::Asset& asset, const fastgltf::Accessor& accessor, const T*& pointer, size_t* count = nullptr)
        {
            HBL2_CORE_ASSERT(accessor.bufferViewIndex.has_value(), "Loadaccessor: no buffer view index provided");

            const fastgltf::BufferView& bufferView = asset.bufferViews[accessor.bufferViewIndex.value()];
            auto& buffer = asset.buffers[bufferView.bufferIndex];

            const fastgltf::sources::Array* vector = std::get_if<fastgltf::sources::Array>(&buffer.data);
            HBL2_CORE_ASSERT(vector, "FastGltfLoader::LoadAccessor: unsupported data type");

            size_t dataOffset = bufferView.byteOffset + accessor.byteOffset;
            pointer = reinterpret_cast<const T*>(vector->bytes.data() + dataOffset);

            if (count)
            {
                *count = accessor.count;
            }
        }

		static thread_local std::vector<Vertex> s_Vertices;
		static thread_local std::vector<uint32_t> s_Indices;
        static thread_local std::unordered_map<const char*, Handle<Asset>> s_MaterialNameToAssetHandle;
	};
}
