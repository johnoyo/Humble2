#include "LinkSystem.h"

namespace HBL2
{
	void LinkSystem::OnCreate()
	{
		Context::ActiveScene->GetRegistry()
			.group<Component::Link>(entt::get<Component::Transform>)
			.each([&](entt::entity entity, Component::Link& link, Component::Transform& transform)
			{
				transform.WorldMatrix = GetWorldSpaceTransform(entity, link);
			});
	}

	void LinkSystem::OnUpdate(float ts)
	{
		Context::ActiveScene->GetRegistry()
			.group<Component::Link>(entt::get<Component::Transform>)
			.each([&](entt::entity entity, Component::Link& link, Component::Transform& transform)
			{
				if (!transform.Static)
				{
					transform.WorldMatrix = GetWorldSpaceTransform(entity, link);
				}
			});
	}

	glm::mat4 LinkSystem::GetWorldSpaceTransform(entt::entity entity, Component::Link& link)
	{
		glm::mat4 transform = glm::mat4(1.0f);

		if (link.parent != entt::null)
		{
			Component::Link& parentLink = Context::ActiveScene->GetComponent<Component::Link>(link.parent);
			transform = GetWorldSpaceTransform(link.parent, parentLink);
		}

		return transform * Context::ActiveScene->GetComponent<Component::Transform>(entity).Matrix;
	}
}
