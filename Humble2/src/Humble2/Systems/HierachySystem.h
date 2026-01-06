#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"

#include <glm\glm.hpp>
#include <glm\gtx\quaternion.hpp>

namespace HBL2
{
	namespace Transform
	{
		void SetLocal();
		void SetLocalTranslation();
		void SetLocalRotation();
		void SetLocalScale();

		void Set();
		void SetTranslation();
		void SetRotation();
		void SetScale();
	}

	class HBL2_API HierachySystem final : public ISystem
	{
	public:
		HierachySystem() { Name = "HierachySystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		
	private:
		glm::mat4 GetWorldSpaceTransform(Entity entity, Component::Link& link);
		glm::mat4 GetWorldSpaceTransform2(Entity entity, Component::Link& link);
		void AddChildren(Entity entity, Component::Link& link);
		void UpdateChildren(Entity entity, Component::Link& link);

	private:
		std::unordered_map<UUID, glm::mat4> m_WorldCache;
	};
}