#pragma once

#include "FileStreamWriter.h"

#include "Asset/Asset.h"
#include "Resources/Handle.h"
#include "Utilities/Collections/FixedHashMap.h"

#include <cstdint>

namespace HBL2
{
	struct AssetPack
	{
		static constexpr uint32_t CurrentVersion = 0;

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
			uint32_t Version = CurrentVersion;
			uint64_t BuildVersion = 0;
		};

		FileHeader Header;
		FixedHashMap<UUID, AssetInfo> Assets;

		bool Serialize(UUID assetUUID, FileStreamWriter& stream);

	private:
		bool SerializeTexture(Asset* asset, FileStreamWriter& stream);
		bool SerializeShader(Asset* asset, FileStreamWriter& stream);
		bool SerializeMaterial(Asset* asset, FileStreamWriter& stream);
		bool SerializeMesh(Asset* asset, FileStreamWriter& stream);
		bool SerializeScene(Asset* asset, FileStreamWriter& stream);
		bool SerializePrefab(Asset* asset, FileStreamWriter& stream);
		bool SerializeScript(Asset* asset, FileStreamWriter& stream);
		bool SerializeSound(Asset* asset, FileStreamWriter& stream);
	};
}