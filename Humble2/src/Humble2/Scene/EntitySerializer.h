#pragma once

#include "Scene.h"

#include <entt.hpp>
#include <yaml-cpp\yaml.h>

namespace HBL2
{
	class EntitySerializer
	{
	public:
		EntitySerializer(Scene* scene, entt::entity entity);

		void Serialize(YAML::Emitter& out);
		bool Deserialize(const YAML::Node& entityNode, bool isPrefab = false);

	private:
		Scene* m_Scene;
		entt::entity m_Entity;
	};
}