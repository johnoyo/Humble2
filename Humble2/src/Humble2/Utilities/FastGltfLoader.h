#pragma once

#include "Base.h"

#include <filesystem>

namespace HBL2
{
	class FastGltfLoader
	{
	public:
		bool Load(const std::filesystem::path& path, std::vector<Vertex>& meshData);
	};
}