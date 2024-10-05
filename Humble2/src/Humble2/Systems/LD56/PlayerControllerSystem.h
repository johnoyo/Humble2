#pragma once

#include "Base.h"

#include "Scene\Scene.h"
#include "Scene\ISystem.h"

#include "Core\Input.h"
#include "Core\Events.h"

namespace LD56
{
	class PlayerControllerSystem final : public HBL2::ISystem
	{
	public:
		PlayerControllerSystem() { Name = "PlayerControllerSystem"; }

		virtual void OnCreate() override
		{
			auto playerEntity = m_Context->FindEntityByUUID(11084216877952962560);

			if (playerEntity != entt::null)
			{
				m_Context->AddComponent<Component::PlayerController>(playerEntity);
			}
		}

		virtual void OnUpdate(float ts) override
		{
			m_Context->GetRegistry()
				.group<Component::PlayerController>(entt::get<HBL2::Component::Transform>)
				.each([&](auto entity, Component::PlayerController& controller, HBL2::Component::Transform& transform)
				{
					float targetX = transform.Translation.x;

					if (HBL2::Input::GetKeyDown(GLFW_KEY_D))
					{
						if (transform.Translation.x <= 26.0f)
						{
							targetX += controller.MovementSpeed * ts;
						}
						else
						{
							targetX = 26.0f;
						}
					}
					else if (HBL2::Input::GetKeyDown(GLFW_KEY_A))
					{
						if (transform.Translation.x >= -24.0f)
						{
							targetX -= controller.MovementSpeed * ts;
						}
						else
						{
							targetX = -24.0f;
						}
					}
					
					// Calculate the velocity change based on the difference between the current and target position
					float desiredVelocityX = (targetX - transform.Translation.x) * controller.MovementSpeed;

					// Smoothly interpolate the velocity for more gradual movement
					controller.Velocity.x = glm::mix(controller.Velocity.x, desiredVelocityX, controller.DampingFactor * ts);

					// Apply the velocity to the position
					transform.Translation.x += controller.Velocity.x * ts;
				});
		}
	};
}