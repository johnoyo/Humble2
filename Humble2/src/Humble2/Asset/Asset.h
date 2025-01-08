#pragma once

#include "Base.h"

#include <fstream>
#include <stdint.h>
#include <filesystem>

namespace HBL2
{
	enum class HBL2_API AssetType
	{
		None = 0,
		Texture = 1,
		Shader = 2,
		Material = 3,
		Mesh = 4,
		Scene = 5,
		Script = 6,
	};

	struct HBL2_API AssetDescriptor
	{
		const char* debugName;
		std::filesystem::path filePath;
		AssetType type;
	};

	struct HBL2_API Asset
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