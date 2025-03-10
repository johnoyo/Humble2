#include "Elements.h"

#include "imgui.h"

namespace HBL2
{
	namespace UI
	{
		void CreatePanel(Config&& config, const std::function<void(Panel*)>&& body)
		{
			Panel panel(std::forward<Config>(config), std::forward<const std::function<void(Panel*)>>(body));
		}

		void CreateEditorPanel(const char* panelName, Config&& config, const std::function<void(Panel*)>&& body)
		{
			Panel panel(panelName, std::forward<Config>(config), std::forward<const std::function<void(Panel*)>>(body));
		}

		void Text(Panel* parent, const char* text, glm::vec4 color)
		{
			const ImVec2& textSize = ImGui::CalcTextSize(text);

			parent->AddChild(UI::Panel{
				UI::Config{
					.id = text,
					.parent = parent,
					.mode = Rectagle{
						.color = { color.r, color.g, color.b, color.a },
					},
					.layout = Layout{
						.sizing = {
							.width = textSize.x,
							.height = textSize.y
						},
						.padding = { 20, 20 },
					},
					.type = ElementType::TEXT,
				},
				[](UI::Panel* parent) {}
			});
		}

		void Image(Panel* parent, const std::string& path, const glm::vec2& size)
		{
			parent->AddChild(UI::Panel{
				UI::Config{
					.id = _strdup(path.c_str()),
					.parent = parent,
					.mode = Rectagle{
						.color = { 0, 0, 0, 0 },
					},
					.layout = Layout{
						.sizing = {
							.width = size.x,
							.height = size.y,
						},
						.padding = { 20, 20 },
					},
					.type = ElementType::IMAGE,
				},
				[](UI::Panel* parent) {}
			});
		}
	}
}
