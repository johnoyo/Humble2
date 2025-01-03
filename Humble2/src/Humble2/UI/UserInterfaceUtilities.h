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
				return *(ImVec2*)&Context::ViewportSize;
			}

			static inline ImVec2 GetWindowPosition()
			{
				return *(ImVec2*)&Context::ViewportPosition;
			}

			static inline float GetFontSize()
			{
				return ImGui::GetFontSize();
			}
		}
	}
}