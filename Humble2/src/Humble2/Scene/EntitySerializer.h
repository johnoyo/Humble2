#pragma once

#include "Scene.h"

#include <entt.hpp>
#include <yaml-cpp\yaml.h>

namespace HBL2
{
	class EntitySerializer
	{
	public:
		EntitySerializer(Scene* scene, Entity entity);

		void Serialize(YAML::Emitter& out);
		bool Deserialize(const YAML::Node& entityNode);

	private:
		Scene* m_Scene;
		Entity m_Entity;
	};
}