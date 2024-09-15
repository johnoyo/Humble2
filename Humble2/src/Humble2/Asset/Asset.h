#pragma once

#include "Base.h"

#include <yaml-cpp\yaml.h>

#include <fstream>
#include<stdint.h>
#include<filesystem>

namespace HBL2
{
	enum class AssetType
	{
		None = 0,
		Texture = 1,
		Shader = 2,
		Material = 3,
		Mesh = 4,
		Scene = 5,
	};

	struct AssetDescriptor
	{
		const char* debugName;
		std::filesystem::path filePath;
		AssetType type;
	};

	struct Asset
	{
		Asset() = default;
		Asset(const AssetDescriptor&& desc)
		{
			DebugName = desc.debugName;
			FilePath = desc.filePath;
			Type = desc.type;

			UUID = std::hash<std::string>()(FilePath.string());
		}

		const char* DebugName = "";
		UUID UUID = 0;
		uint32_t Indentifier = 0;
		AssetType Type = AssetType::None;
		std::filesystem::path FilePath;
		uint32_t FileFormatVersion = 1;
		bool Loaded = false;
	};
}