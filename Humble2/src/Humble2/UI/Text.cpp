#include "Text.h"

namespace HBL2
{
	namespace UI
	{
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
				[&](UI::Panel* parent) {}
			});
		}
	}
}
