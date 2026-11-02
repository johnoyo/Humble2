#include "AssetPackSerializer.h"

#include "FileStreamReader.h"
#include "FileStreamWriter.h"

namespace HBL2
{
	AssetPackSerializer::AssetPackSerializer(AssetPack* assetPack)
		: m_Context(assetPack)
	{

	}

	void AssetPackSerializer::Serialize(const std::filesystem::path& path, std::atomic<float>& progress)
	{
		HBL2_CORE_TRACE("Serializing AssetPack to {}", path.string());
		HBL2_CORE_TRACE("  {} assets", m_Context->Assets.size());

		FileStreamWriter serializer(path);
		serializer.WriteRaw<AssetPack::FileHeader>(m_Context->Header);

		// Write dummy data for index (come back later to fill in)
		uint64_t indexPos = serializer.GetStreamPosition();
		uint64_t indexTableSize = CalculateIndexTableSize();
		serializer.WriteZero(indexTableSize);

		float progressIncrement = 0.4f / (float)m_Context->Assets.size();

		// Write asset data + fill in offset + size
		for (auto& [assetUUID, assetInfo] : m_Context->Assets)
		{
			// Serialize asset.
			AssetSerializationInfo serializationInfo;

			if (AssetPack::Serialize(assetUUID, serializer, serializationInfo))
			{
				m_Context->Assets[assetUUID].PackedOffset = serializationInfo.Offset;
				m_Context->Assets[assetUUID].PackedSize = serializationInfo.Size;
			}
			else
			{
				HBL2_CORE_ERROR("Failed to serialize asset with handle {}", assetUUID);

				m_Context->Assets[assetUUID].PackedOffset = 0;
				m_Context->Assets[assetUUID].PackedSize = 0;
			}

			progress = progress + progressIncrement;
		}

		HBL2_CORE_TRACE("Serialized {} assets into AssetPack", m_Context->Assets.size());

		// Reset cursor position.
		serializer.SetStreamPosition(indexPos);
		
		// Write asset map data.
		serializer.WriteMap(m_Context->Assets);

		progress = progress + 0.1f;
	}

	bool AssetPackSerializer::DeserializeIndex(const std::filesystem::path& path)
	{
		HBL2_CORE_TRACE("[AssetPack] Deserializing Asset Pack from {}", path.string());

		FileStreamReader stream(path);
		if (!stream.IsStreamGood())
		{
			return false;
		}

		stream.ReadRaw<AssetPack::FileHeader>(m_Context->Header);
		bool validHeader = memcmp(m_Context->Header.Header, "HBLAP", 4) == 0;
		HBL2_CORE_ASSERT(validHeader, "Invalid asset pack header, aborting asset pack load!");
		if (!validHeader)
		{
			return false;
		}

		AssetPack current;
		if (m_Context->Header.Version != current.Header.Version)
		{
			HBL2_CORE_ERROR("AssetPack version {} is not compatible with current version {}", m_Context->Header.Version, current.Header.Version);
			return false;
		}

		// Read scene map size
		uint32_t assetCount = 0;
		stream.ReadRaw<uint32_t>(assetCount);
		
		for (uint32_t i = 0; i < assetCount; i++)
		{
			uint64_t assetUUID = 0;
			stream.ReadRaw<uint64_t>(assetUUID);

			AssetPack::AssetInfo& assetInfo = m_Context->Assets[assetUUID];
			stream.ReadRaw<uint64_t>(assetInfo.PackedOffset);
			stream.ReadRaw<uint64_t>(assetInfo.PackedSize);
			stream.ReadRaw<uint16_t>(assetInfo.Type);
			stream.ReadRaw<uint16_t>(assetInfo.Flags);
		}

		HBL2_CORE_TRACE("[AssetPack] Deserialized index with {} assets from AssetPack", assetCount);
		return true;
	}

	uint64_t AssetPackSerializer::CalculateIndexTableSize()
	{
		return sizeof(uint32_t) + (sizeof(UUID) + sizeof(AssetPack::AssetInfo)) * m_Context->Assets.size();
	}
}