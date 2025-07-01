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
		PrefabSerializer(Prefab* prefab, Scene* scene);
		PrefabSerializer(Prefab* prefab, entt::entity instantiatedPrefabEntity);

		void Serialize(const std::filesystem::path& path);
		bool Deserialize(const std::filesystem::path& path);

	private:
		void SerializePrefab(Scene* ctx, entt::entity entity, YAML::Emitter& out);
		Scene* GetScene();

	private:
		Prefab* m_Context;
		Scene* m_Scene = nullptr;
		entt::entity m_InstantiatedPrefabEntity = entt::null;
	};
}