#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"

namespace HBL2
{
	class LinkSystem final : public ISystem
	{
	public:
		LinkSystem() { Name = "LinkSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
	private:
		glm::mat4 GetWorldSpaceTransform(entt::entity entity, Component::Link& link);
	};
}