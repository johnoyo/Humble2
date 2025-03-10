#pragma once

#include "Base.h"
#include "Panel.h"

namespace HBL2
{
	namespace UI
	{
		HBL2_API void CreatePanel(Config&& config, const std::function<void(Panel*)>&& body);
		HBL2_API void CreateEditorPanel(const char* panelName, Config&& config, const std::function<void(Panel*)>&& body);
		HBL2_API void Text(Panel* parent, const char* text, glm::vec4 color = { 255, 255, 255, 255 });
		HBL2_API void Image(Panel* parent, const std::string& path, const glm::vec2& size = { 50.0f, 50.0f });
	}
}