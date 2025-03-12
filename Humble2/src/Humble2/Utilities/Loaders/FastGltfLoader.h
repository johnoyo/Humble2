#pragma once

#include "Base.h"
#include "Resources\Types.h"

#include <filesystem>

namespace HBL2
{
	class FastGltfLoader
	{
	public:
		Handle<Mesh> Load(const std::filesystem::path& path);
	};
}