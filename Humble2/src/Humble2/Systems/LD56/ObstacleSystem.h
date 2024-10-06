#pragma once

#include "Base.h"

#include "Scene\Scene.h"
#include "Scene\ISystem.h"

namespace LD56
{
	class ObstacleSystem final : public HBL2::ISystem
	{
	public:
		ObstacleSystem() { Name = "ObstacleSystem"; }

		virtual void OnCreate() override
		{
			m_Context->GetRegistry()
				.view<HBL2::Component::Tag>()
				.each([&](auto entity, HBL2::Component::Tag& tag)
				{
					if (tag.Name.find("Collider_") != std::string::npos)
					{
						Component::Obstacle& obstacle = m_Context->AddComponent<Component::Obstacle>(entity);
					}
				});

			m_Player = m_Context->FindEntityByUUID(11084216877952962560);

			if (m_Player == entt::null)
			{
				HBL2_CORE_INFO("Player entity not found!");
			}
		}

		virtual void OnUpdate(float ts) override
		{
			HBL2::Component::Transform& playerTransform = m_Context->GetComponent<HBL2::Component::Transform>(m_Player);

			m_Context->GetRegistry()
				.group<Component::Obstacle>(entt::get<HBL2::Component::Transform>)
				.each([&](auto entity, Component::Obstacle& obstacle, HBL2::Component::Transform& transform)
				{
					if (obstacle.Enabled)
					{
						glm::vec4 wolrdPosition = transform.WorldMatrix * glm::vec4(transform.Translation, 1.0f);
						glm::vec2 obstacleCenter = { wolrdPosition.x, wolrdPosition.z };
						glm::vec2 obstacleScale = { transform.Scale.x, transform.Scale.y };
						glm::vec2 playerPosition = { playerTransform.Translation.x, playerTransform.Translation.z };

						if (obstacleCenter.y + (obstacleScale.y * 0.5f) >= 15.0f - (obstacleScale.y - 1.0f) && obstacleCenter.y - (obstacleScale.y * 0.5f) <= 15.0f - (obstacleScale.y - 1.0f))
						{
							if (playerPosition.x <= transform.Translation.x + (obstacleScale.x * 0.5f) && playerTransform.Translation.x >= transform.Translation.x - (obstacleScale.x * 0.5f))
							{
								m_Context->GetComponent<Component::PlayerController>(m_Player).Alive = false;
							}
						}
					}
				});
		}

	private:
		entt::entity m_Player;
	};
}