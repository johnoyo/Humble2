#include "AssetPack.h"

#include "Asset/EditorAssetManager.h"

namespace HBL2
{
    bool AssetPack::Serialize(UUID assetUUID, FileStreamWriter& stream, AssetSerializationInfo& outInfo)
    {
        EditorAssetManager* assetManager = (EditorAssetManager*)AssetManager::Instance;

        Handle<Asset> assetHandle = assetManager->GetHandleFromUUID(assetUUID);
        Asset* asset = assetManager->GetAssetMetadata(assetHandle);

        switch (asset->Type)
        {
		case AssetType::Texture:
			return SerializeTexture(asset, stream, outInfo);
		case AssetType::Shader:
			return SerializeShader(asset, stream, outInfo);
		case AssetType::Material:
			return SerializeMaterial(asset, stream, outInfo);
		case AssetType::Mesh:
			return SerializeMesh(asset, stream, outInfo);
		case AssetType::Scene:
			return SerializeScene(asset, stream, outInfo);
		case AssetType::Script:
			return SerializeScript(asset, stream, outInfo);
		case AssetType::Sound:
			return SerializeSound(asset, stream, outInfo);
		case AssetType::Prefab:
			return SerializePrefab(asset, stream, outInfo);
		case AssetType::None:
			return false;
        }

        return false;
    }

	bool AssetPack::SerializeTexture(Asset* asset, FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		return false;
	}

	bool AssetPack::SerializeShader(Asset* asset, FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		return false;
	}

	bool AssetPack::SerializeMaterial(Asset* asset, FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		return false;
	}

	bool AssetPack::SerializeMesh(Asset* asset, FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		return false;
	}

	bool AssetPack::SerializeScene(Asset* asset, FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		return false;
	}

	bool AssetPack::SerializePrefab(Asset* asset, FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		return false;
	}

	bool AssetPack::SerializeScript(Asset* asset, FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		return false;
	}

	bool AssetPack::SerializeSound(Asset* asset, FileStreamWriter& stream, AssetSerializationInfo& outInfo)
	{
		return false;
	}
}
