#pragma once

#include "imgui.h"

namespace HBL2
{
	namespace UI
	{
		namespace Utils
		{
			static inline ImVec2 GetWindowSize()
			{
				return ImGui::GetMainViewport()->Size;
			}

			static inline float GetFontSize()
			{
				return ImGui::GetFontSize();
			}
		}
	}
}