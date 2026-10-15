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
    struct SkinVertex
    {
        uint16_t Joints[4] = { 0, 0, 0, 0 };
        float Weights[4] = { 1.f, 0.f, 0.f, 0.f };
    };

    struct SkeletonImport
    {
        bool Valid = false;
        uint32_t SkinIndex = 0;
        std::vector<int32_t> NodeToJoint;      // glTF node -> final joint index, -1 if not a joint
        std::vector<uint16_t> SkinJointRemap;  // skin.joints[k] (what JOINTS_0 indexes) -> final joint index
        SkeletonDescriptor Descriptor;
    };

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

        SkeletonImport BuildSkeleton(const std::filesystem::path& path, const fastgltf::Asset& asset);
        AnimationSampler BuildSampler(const fastgltf::Asset& asset, const fastgltf::AnimationSampler& src);
        void BuildAnimationClips(const fastgltf::Asset& asset, const SkeletonImport& skeleton, std::vector<AnimationClipDescriptor>& out);
        void CreateSkeletonAndClips(const fastgltf::Asset& asset, SkeletonImport& import, Handle<Skeleton>& skeleton, std::vector<Handle<AnimationClip>>& clips);

		Result<MeshPartDescriptor> LoadMeshData(const fastgltf::Asset& asset, const fastgltf::Node& node, uint32_t meshIndex, const SkeletonImport& skeleton);
		Result<SubMeshDescriptor> LoadSubMeshVertexData(const fastgltf::Asset& asset, const fastgltf::Mesh& mesh, uint32_t subMeshIndex, bool skinned, const SkeletonImport& skeleton);

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
		static thread_local std::vector<SkinVertex> s_SkinVertices;
        static thread_local std::unordered_map<const char*, Handle<Asset>> s_MaterialNameToAssetHandle;
	};
}
