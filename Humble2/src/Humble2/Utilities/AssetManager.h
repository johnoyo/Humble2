#pragma once

#include "Base.h"
#include "Random.h"
#include "Project\Project.h"

#include <yaml-cpp\yaml.h>

#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <type_traits>

namespace HBL
{
	struct Asset
	{
		UUID UUID;
		std::string Name;
		std::filesystem::path Path;
		std::filesystem::path RelativePath;
		uint32_t FileFormatVersion = 1;
		void* UserData = nullptr;
	};


	class AssetManager
	{
	public:
		AssetManager(const AssetManager&) = delete;

		static AssetManager& Get()
		{
			static AssetManager instance;
			return instance;
		}

		UUID CreateAsset(const std::filesystem::path& path, void* userData = nullptr)
		{
			UUID uuid = std::hash<std::string>()(path.string());

			if (AssetExists(uuid))
			{
				return uuid;
			}

			Asset* asset = new Asset;
			asset->UUID = uuid;
			asset->Name = path.filename().stem().string();
			asset->Path = path;
			asset->RelativePath = std::filesystem::relative(path, HBL2::Project::GetAssetDirectory());
			asset->UserData = userData;

			Serialize(asset);

			m_AssetRegistry[uuid] = asset;

			return uuid;
		}

		Asset* GetAsset(UUID uuid)
		{
			if (m_AssetRegistry.find(uuid) == m_AssetRegistry.end())
			{
				return nullptr;
			}

			return m_AssetRegistry[uuid];
		}

		bool SaveAsset(UUID uuid)
		{
			if (AssetExists(uuid))
			{
				Asset* asset = m_AssetRegistry[uuid];
				asset->FileFormatVersion++;
				Serialize(asset);
				return true;
			}

			return false;
		}

		bool DeleteAsset(UUID guid)
		{
			return false;
		}

	private:
		void Serialize(Asset* asset);

		bool AssetExists(UUID uuid) const
		{
			return m_AssetRegistry.find(uuid) != m_AssetRegistry.end();
		}

	private:
		AssetManager() = default;
		std::unordered_map<UUID, Asset*> m_AssetRegistry;
	};
}
