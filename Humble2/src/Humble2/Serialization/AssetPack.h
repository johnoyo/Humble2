#pragma once

#include "FileStreamWriter.h"

#include "Asset/Asset.h"
#include "Utilities/Collections/FixedHashMap.h"

#include <cstdint>

namespace HBL2
{
	struct AssetSerializationInfo
	{
		uint64_t Offset = 0;
		uint64_t Size = 0;
	};

	struct AssetPack
	{
		struct AssetInfo
		{
			uint64_t PackedOffset = 0;
			uint64_t PackedSize = 0;
			uint16_t Type = 0;
			uint16_t Flags = 0;
		};

		struct FileHeader
		{
			const char Header[5] = { 'H', 'B', 'L', 'A', 'P' };
			uint32_t Version = 0;
			uint64_t BuildVersion = 0;
		};

		FileHeader Header;
		FixedHashMap<UUID, AssetInfo> Assets;

		static bool Serialize(UUID assetUUID, FileStreamWriter& stream, AssetSerializationInfo& outInfo);

	private:
		static bool SerializeTexture(Asset* asset, FileStreamWriter& stream, AssetSerializationInfo& outInfo);
		static bool SerializeShader(Asset* asset, FileStreamWriter& stream, AssetSerializationInfo& outInfo);
		static bool SerializeMaterial(Asset* asset, FileStreamWriter& stream, AssetSerializationInfo& outInfo);
		static bool SerializeMesh(Asset* asset, FileStreamWriter& stream, AssetSerializationInfo& outInfo);
		static bool SerializeScene(Asset* asset, FileStreamWriter& stream, AssetSerializationInfo& outInfo);
		static bool SerializePrefab(Asset* asset, FileStreamWriter& stream, AssetSerializationInfo& outInfo);
		static bool SerializeScript(Asset* asset, FileStreamWriter& stream, AssetSerializationInfo& outInfo);
		static bool SerializeSound(Asset* asset, FileStreamWriter& stream, AssetSerializationInfo& outInfo);
	};
}