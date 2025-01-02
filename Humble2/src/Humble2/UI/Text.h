#pragma once

#include "Base.h"

#include "imgui.h"

namespace HBL2
{
	namespace UI
	{
		struct TextConfiguration
		{
			float fontSize = 12;
			glm::vec4 color = { 255, 255, 255, 255 };
		};

		class Text
		{
		public:
			Text(const char* text, TextConfiguration&& config = {})
			{
				ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(config.color.r, config.color.g, config.color.b, config.color.a));

				ImGui::Text(text);

				ImGui::PopStyleColor();
			}
		};
	}
}