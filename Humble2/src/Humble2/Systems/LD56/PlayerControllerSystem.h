#pragma once

#include "Base.h"

#include "Scene\Scene.h"
#include "Scene\ISystem.h"

#include "Core\Input.h"
#include "Core\Events.h"

#include <imgui.h>

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
					if (controller.Alive)
					{
						float targetX = transform.Translation.x;

						if (HBL2::Input::GetKeyDown(GLFW_KEY_D))
						{
							if (transform.Translation.x <= 26.0f)
							{
								targetX += controller.MovementSpeed;
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
								targetX -= controller.MovementSpeed;
							}
							else
							{
								targetX = -24.0f;
							}
						}
					
						float desiredVelocityX = (targetX - transform.Translation.x) * controller.MovementSpeed;
						controller.Velocity.x = glm::mix(controller.Velocity.x, desiredVelocityX, controller.DampingFactor * ts);
						transform.Translation.x += controller.Velocity.x * ts;

						m_Score += controller.MovementSpeed * 10.0f * ts;
					}
				});
		}

		virtual void OnGuiRender(float ts) override
		{
			ImGui::SetNextWindowPos(ImVec2(15, 15));
			ImGui::SetNextWindowSize(ImVec2(150, 32));

			ImGui::Begin("Score", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
			ImGui::Text(std::format("Score {}", (uint32_t)m_Score).c_str());
			ImGui::End();
		}

	private:
		float m_Score = 0;
	};
}