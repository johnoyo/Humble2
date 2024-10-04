#pragma once

#include "Base.h"

#include <filesystem>

namespace HBL2
{
	struct MeshData;

	class FastGltfLoader
	{
	public:
		bool Load(const std::filesystem::path& path, MeshData& meshData);
	};
}