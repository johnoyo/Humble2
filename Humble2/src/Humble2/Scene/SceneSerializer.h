#pragma once

#include "Scene.h"

#include <fstream>
#include <yaml-cpp\yaml.h>

namespace HBL2
{
	class SceneSerializer
	{
	public:
		SceneSerializer(Scene* scene);

		void Serialize(const std::string& filePath);
		bool Deserialize(const std::string& filePath);
	private:
		void SerializeEntity(YAML::Emitter& out, entt::entity entity);
	private:
		Scene* m_Scene = nullptr;
	};
}