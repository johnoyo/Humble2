#pragma once

#include "Base.h"

#include <filesystem>

namespace HBL2
{
	struct MeshData
	{
		struct Vertex
		{
			glm::vec3 Position;
			glm::vec3 Normal;
			glm::vec2 TextureCoord;
		};

		std::vector<Vertex> Data;
		bool Result = false;
	};

	class MeshUtilities
	{
	public:
		MeshUtilities(const MeshUtilities&) = delete;

		static MeshUtilities& Get()
		{
			static MeshUtilities instance;
			return instance;
		}

		MeshData Load(const std::string& path);

	private:
		MeshUtilities() = default;
	};
}