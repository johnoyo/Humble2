#pragma once

#include "Prefab.h"
#include "Scene\Scene.h"

#include <yaml-cpp\yaml.h>

#include <fstream>
#include <filesystem>

namespace HBL2
{
	class PrefabSerializer
	{
	public:
		PrefabSerializer(Prefab* prefab);

		void Serialize(const std::filesystem::path& path);
		bool Deserialize(const std::filesystem::path& path);

	private:
		void SerializePrefab(Scene* ctx, entt::entity entity, YAML::Emitter& out);

	private:
		Prefab* m_Context;
	};
}