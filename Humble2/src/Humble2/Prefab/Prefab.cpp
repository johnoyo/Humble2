#include "Prefab.h"

#include <Project/Project.h>

namespace HBL2
{
	void Prefab::CreateMetadataFile(Handle<Asset> assetHandle, UUID baseEntityUUID)
	{
		Asset* prefabAsset = AssetManager::Instance->GetAssetMetadata(assetHandle);

		CreateMetadataFile(prefabAsset, baseEntityUUID);
	}

	void Prefab::CreateMetadataFile(Asset* prefabAsset, UUID baseEntityUUID)
	{
		if (prefabAsset == nullptr)
		{
			return;
		}

		const auto& relativePath = prefabAsset->FilePath;

		std::ofstream fout(HBL2::Project::GetAssetFileSystemPath(relativePath).string() + ".hblprefab", 0);
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Prefab" << YAML::Value;
		out << YAML::BeginMap;
		out << YAML::Key << "UUID" << YAML::Value << prefabAsset->UUID;
		out << YAML::Key << "BaseEntityUUID" << YAML::Value << baseEntityUUID;
		out << YAML::EndMap;
		out << YAML::EndMap;
		fout << out.c_str();
		fout.close();
	}
}