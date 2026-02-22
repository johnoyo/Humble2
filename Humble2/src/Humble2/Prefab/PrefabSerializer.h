#pragma once

#include "Prefab.h"
#include "Scene\Scene.h"

#include <yaml-cpp\yaml.h>

#include <fstream>
#include <filesystem>

namespace HBL2
{
	class HBL2_API PrefabSerializer
	{
	public:
		PrefabSerializer(Prefab* prefab);
		PrefabSerializer(Prefab* prefab, Entity instantiatedPrefabEntity);

		void Serialize(const std::filesystem::path& path);
		bool Deserialize(const std::filesystem::path& path);

	private:
		void SerializePrefab(Scene* ctx, Entity entity, YAML::Emitter& out);

	private:
		Prefab* m_Context;
		Entity m_InstantiatedPrefabEntity = Entity::Null;
	};
}