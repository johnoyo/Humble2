#include "AssetPack.h"

#include "Asset/EditorAssetManager.h"

namespace HBL2
{
    bool AssetPack::Serialize(UUID assetUUID, FileStreamWriter& stream)
    {
        EditorAssetManager* assetManager = (EditorAssetManager*)AssetManager::Instance;

        Handle<Asset> assetHandle = assetManager->GetHandleFromUUID(assetUUID);
        Asset* asset = assetManager->GetAssetMetadata(assetHandle);

		bool result = false;

        switch (asset->Type)
        {
		case AssetType::Texture:
			result = SerializeTexture(asset, stream);
			break;
		case AssetType::Shader:
			result = SerializeShader(asset, stream);
			break;
		case AssetType::Material:
			result = SerializeMaterial(asset, stream);
			break;
		case AssetType::Mesh:
			result = SerializeMesh(asset, stream);
			break;
		case AssetType::Scene:
			result = SerializeScene(asset, stream);
			break;
		case AssetType::Script:
			result = SerializeScript(asset, stream);
			break;
		case AssetType::Sound:
			result = SerializeSound(asset, stream);
			break;
		case AssetType::Prefab:
			result = SerializePrefab(asset, stream);
			break;
		case AssetType::None:
			result = false;
			break;
        }

        return result;
    }

	bool AssetPack::SerializeTexture(Asset* asset, FileStreamWriter& stream)
	{
		return false;
	}

	bool AssetPack::SerializeShader(Asset* asset, FileStreamWriter& stream)
	{
		return false;
	}

	bool AssetPack::SerializeMaterial(Asset* asset, FileStreamWriter& stream)
	{
		return false;
	}

	bool AssetPack::SerializeMesh(Asset* asset, FileStreamWriter& stream)
	{
		return false;
	}

	bool AssetPack::SerializeScene(Asset* asset, FileStreamWriter& stream)
	{
		return false;
	}

	bool AssetPack::SerializePrefab(Asset* asset, FileStreamWriter& stream)
	{
		return false;
	}

	bool AssetPack::SerializeScript(Asset* asset, FileStreamWriter& stream)
	{
		return false;
	}

	bool AssetPack::SerializeSound(Asset* asset, FileStreamWriter& stream)
	{
		return false;
	}
}
