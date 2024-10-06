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
					if (tag.Name.find("Obstacle_") != std::string::npos)
					{
						Component::Obstacle& obstacle = m_Context->AddComponent<Component::Obstacle>(entity);

						if (tag.Name.find("Type0") != std::string::npos)
						{
							obstacle.ExtentX = 5.75f;
							obstacle.ExtentY = 6.0f;
						}
						else if (tag.Name.find("Type1") != std::string::npos)
						{
							obstacle.ExtentX = 5.0f;
							obstacle.ExtentY = 5.0f;
						}
						else if (tag.Name.find("Type2") != std::string::npos)
						{
							obstacle.ExtentX = 8.5f;
							obstacle.ExtentY = 8.5f;
						}
						else if (tag.Name.find("Type3") != std::string::npos)
						{
							obstacle.ExtentX = 8.0f;
							obstacle.ExtentY = 4.0f;
						}
						else if (tag.Name.find("Type4") != std::string::npos)
						{
							obstacle.ExtentX = 11.5f;
							obstacle.ExtentY = 8.0f;
						}
						else if (tag.Name.find("Type5") != std::string::npos)
						{
							obstacle.ExtentX = 9.0f;
							obstacle.ExtentY = 15.0f;
						}
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

						obstacle.TopRight.x = wolrdPosition.x + obstacle.ExtentX;
						obstacle.TopRight.y = wolrdPosition.z - obstacle.ExtentY / 2.0f;

						obstacle.BottomRight.x = wolrdPosition.x + obstacle.ExtentX;
						obstacle.BottomRight.y = wolrdPosition.z + obstacle.ExtentY;

						obstacle.BottomLeft.x = wolrdPosition.x - obstacle.ExtentX;
						obstacle.BottomLeft.y = wolrdPosition.z + obstacle.ExtentY;

						obstacle.TopLeft.x = wolrdPosition.x - obstacle.ExtentX;
						obstacle.TopLeft.y = wolrdPosition.z - obstacle.ExtentY / 2.0f;

						glm::vec2 playerPosition = { playerTransform.Translation.x, playerTransform.Translation.z };

						if (IsPointInQuad(playerPosition, obstacle.TopRight, obstacle.BottomRight, obstacle.BottomLeft, obstacle.TopLeft))
						{
							HBL2_INFO("Player hit obstacle: {}.", m_Context->GetComponent<HBL2::Component::Tag>(entity).Name);
							// m_Context->GetComponent<Component::PlayerController>(m_Player).Alive = false;
						}
					}
				});
		}

	private:
		bool IsPointInTriangle(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, const glm::vec2& c)
		{
			glm::vec2 v0 = c - a;
			glm::vec2 v1 = b - a;
			glm::vec2 v2 = p - a;

			float dot00 = glm::dot(v0, v0);
			float dot01 = glm::dot(v0, v1);
			float dot02 = glm::dot(v0, v2);
			float dot11 = glm::dot(v1, v1);
			float dot12 = glm::dot(v1, v2);

			float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
			float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
			float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

			return (u >= 0) && (v >= 0) && (u + v <= 1);
		}

		bool IsPointInQuad(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, const glm::vec2& c, const glm::vec2& d)
		{
			return IsPointInTriangle(p, a, b, c) || IsPointInTriangle(p, a, c, d);
		}

	private:
		entt::entity m_Player;
	};
}