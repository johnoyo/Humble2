#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"

#include <glm\glm.hpp>
#include <glm\gtx\quaternion.hpp>

namespace HBL2
{
	class HBL2_API HierachySystem final : public ISystem
	{
	public:
		HierachySystem() { Name = "HierachySystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		void OnUpdate2(float ts);
		
	private:
		glm::mat4 GetWorldSpaceTransform(Entity entity, Component::Link& link);
		void AddChildren(Entity entity, Component::Link& link);
		void UpdateChildren(Entity entity, Component::Link& link);

		void MarkDirtyRecursive(Entity e);
		void ComputeWorldFromDirtyRoots();
	};
}