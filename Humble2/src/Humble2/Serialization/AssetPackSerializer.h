#pragma once

#include "AssetPack.h"

#include <filesystem>

namespace HBL2
{
	class AssetPackSerializer
	{
	public:
		AssetPackSerializer(AssetPack* assetPack);

		void Serialize(const std::filesystem::path& path, std::atomic<float>& progress);
		bool Deserialize(const std::filesystem::path& path);

	private:
		uint64_t CalculateIndexTableSize();

	private:
		AssetPack* m_Context;
	};
}