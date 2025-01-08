#pragma once

#include "Base.h"

#include "Panel.h"

#include "imgui.h"

namespace HBL2
{
	namespace UI
	{
		HBL2_API void Text(Panel* parent, const char* text, glm::vec4 color = { 255, 255, 255, 255 });
	}
}