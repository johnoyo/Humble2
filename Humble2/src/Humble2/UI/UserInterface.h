#pragma once

#include "Base.h"

#include <vector>
#include <initializer_list>

#include "imgui.h"

#define CONTENTS []()

namespace HBL2
{
	namespace UI
	{
		extern Offset g_Offset;

		enum class LayoutDirection
		{
			TOP_TO_BOTTOM = 0,
			BOTTOM_TO_TOP,
			LEFT_TO_RIGHT,
			RIGHT_TO_LEFT,
		};

		struct Rectagle
		{
			glm::vec4 color;
			float cornerRadius = 0;
		};

		struct Sizing
		{
			float width;
			float height;
		};

		struct Alignment
		{
			float x;
			float y;
		};

		struct Layout
		{
			LayoutDirection layoutDirection = LayoutDirection::TOP_TO_BOTTOM;
			Sizing sizing;
			glm::vec2 padding;
			float childGap;
			Alignment childAlignment;
		};

		struct Configuration
		{
			const char* ID;
			Rectagle mode;
			Layout layout;
		};

		class UserInterface
		{
		public:
			UserInterface() : m_Valid(false) {}
			UserInterface(Configuration&& config, Body&& body)
				: m_Configuration(std::move(config)), m_Body(std::move(body)), m_Valid(true)
			{
				HBL2_CORE_INFO("UserInterface ctor called");

				Render();
			}

			void Render()
			{
				if (!m_Valid)
				{
					return;
				}

				HBL2_CORE_INFO("ID: {}", m_Configuration.ID);

				const ImVec2 windowSize = ImVec2(m_Configuration.layout.sizing.width, m_Configuration.layout.sizing.height);
				ImVec2 windowPos = ImVec2(g_Offset.x, g_Offset.y);

				ImGuiIO& io = ImGui::GetIO();
				ImVec2 screenCenter = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
				//windowPos = ImVec2((screenCenter.x - windowSize.x * 0.5f) + g_Offset.x, (screenCenter.y - windowSize.y * 0.5f) + g_Offset.y);

				g_Offset.x += m_Configuration.layout.padding.x;
				g_Offset.y += m_Configuration.layout.padding.y + m_Configuration.layout.padding.y;

				ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
				ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, m_Configuration.mode.cornerRadius);
				ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(m_Configuration.mode.color.r, m_Configuration.mode.color.g, m_Configuration.mode.color.b, m_Configuration.mode.color.a));

				ImGui::Begin(m_Configuration.ID, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);				

				m_Body.Render();

				ImGui::End();

				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
				
				switch (m_Configuration.layout.layoutDirection)
				{
				case LayoutDirection::TOP_TO_BOTTOM:
					g_Offset.y += windowSize.y + m_Configuration.layout.padding.y;
					break;
				case LayoutDirection::LEFT_TO_RIGHT:
					g_Offset.x += windowSize.x + m_Configuration.layout.padding.x;
					break;
				}
			}

			void Invalidate()
			{
				g_Offset.x = 0.0f;
				g_Offset.y = 0.0f;
			}

			inline static UserInterface Empty()
			{
				return UserInterface();
			}

		private:
			Configuration m_Configuration{};
			Body m_Body{};
			bool m_Valid = true;
		};
	}
}