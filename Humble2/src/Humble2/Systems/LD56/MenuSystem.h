#pragma once

#include "Base.h"

#include "Project\Project.h"
#include "Scene\Scene.h"
#include "Scene\ISystem.h"

#include "Core\Input.h"
#include "Core\Events.h"

#include <imgui.h>

namespace LD56
{
	class MenuSystem final : public HBL2::ISystem
	{
	public:
		MenuSystem() { Name = "MenuSystem"; }

		virtual void OnCreate() override
		{
			for (ISystem* system : m_Context->GetRuntimeSystems())
			{
				if (system->GetType() == HBL2::SystemType::User && system != this)
				{
					system->SetState(HBL2::SystemState::Pause);
				}
			}

			m_Player = m_Context->FindEntityByUUID(11084216877952962560);

			if (m_Player == entt::null)
			{
				HBL2_CORE_INFO("Player entity not found!");
			}
		}

		virtual void OnUpdate(float ts) override
		{
			if (!m_Context->GetComponent<Component::PlayerController>(m_Player).Alive)
			{
				m_GameStarted = false;
				m_Restart = true;
			}
		}

		virtual void OnGuiRender(float ts) override
		{
			if (!m_GameStarted)
			{
				const ImVec2 windowSize = ImVec2(300.0f, 200.0f);
				const ImVec2 buttonSize = ImVec2(100.0f, 50.0f);

				ImGuiIO& io = ImGui::GetIO();
				ImVec2 screenCenter = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
				ImVec2 windowPos = ImVec2(screenCenter.x - windowSize.x * 0.5f, screenCenter.y - windowSize.y * 0.5f);

				ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
				ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

				ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

				// Center the buttons and label horizontally in the window
				float buttonOffsetX = (windowSize.x - buttonSize.x) * 0.5f;

				// Center the text horizontally
				float labelOffsetX = (windowSize.x - ImGui::CalcTextSize("Run Ant Run").x) * 0.5f;

				// Add some vertical spacing before the label
				ImGui::SetCursorPosY((windowSize.y - (buttonSize.y * 2.0f + 30.0f)) * 0.5f);

				// Center the label horizontally and render the "Game" text
				ImGui::SetCursorPosX(labelOffsetX);
				ImGui::Text("Run Ant Run");

				// Add some vertical space before the first button (Play)
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);

				// Center the button horizontally
				ImGui::SetCursorPosX(buttonOffsetX);
				if (!m_Restart)
				{
					if (ImGui::Button("Play", buttonSize))
					{
						for (ISystem* system : m_Context->GetRuntimeSystems())
						{
							if (system->GetType() == HBL2::SystemType::User)
							{
								system->SetState(HBL2::SystemState::Play);
							}
						}
						m_GameStarted = true;
					}
				}

				// Add some vertical spacing between buttons
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);

				// Center the button horizontally
				ImGui::SetCursorPosX(buttonOffsetX);
				if (ImGui::Button("Exit", buttonSize))
				{
					HBL2::Window::Instance->Close();
				}

				ImGui::End();
			}
		}

	private:
		bool m_GameStarted = false;
		bool m_Restart = false;
		entt::entity m_Player;
	};
}