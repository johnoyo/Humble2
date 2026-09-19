#include "Asset.h"

#include "Utilities/Random.h"

namespace HBL2
{
	Asset::Asset(const AssetDescriptor&& desc)
	{
		DebugName = desc.debugName;
		FilePath = desc.filePath;
		Type = desc.type;
		Pinned = desc.Pin;
	}

	Asset::Asset(const MemoryOnlyAssetDescriptor&& desc)
	{
		DebugName = desc.debugName;
		FilePath = std::filesystem::path("");
		Type = desc.type;
		Indentifier = desc.PackedAssetResourceHandle;
		Pinned = desc.Pin;

		UUID = Random::UInt64();

		// Mark memory only asset immidiately as loaded
		// so that the asset manager will not try and load it.
		Loaded = true;
	}
}
