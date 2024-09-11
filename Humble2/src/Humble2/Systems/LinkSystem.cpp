#include "LinkSystem.h"

namespace HBL2
{
	void LinkSystem::OnCreate()
	{
		m_Context->GetRegistry()
			.group<Component::Link>(entt::get<Component::Transform>)
			.each([&](entt::entity entity, Component::Link& link, Component::Transform& transform)
			{
				transform.WorldMatrix = GetWorldSpaceTransform(entity, link);
			});
	}

	void LinkSystem::OnUpdate(float ts)
	{
		m_Context->GetRegistry()
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
			Component::Link& parentLink = m_Context->GetComponent<Component::Link>(link.parent);
			transform = GetWorldSpaceTransform(link.parent, parentLink);
		}

		return transform * m_Context->GetComponent<Component::Transform>(entity).Matrix;
	}
}
