#pragma once

#include "Base.h"

#include "Panel.h"

namespace HBL2
{
	namespace UI
	{
		class Image
		{
		public:
			Image(Panel* parent, const std::string path)
			{
				parent->AddChild(UI::Panel{
					UI::Config{
						.id = path.c_str(),
						.parent = parent,
						.mode = Rectagle{
							.color = { 0, 0, 0, 0 },
						},
						.layout = Layout{
							.sizing = {
								.width = 50,
								.height = 50
							},
							.padding = { 20, 20 },
						},
						.type = ElementType::IMAGE,
					},
					[&](UI::Panel* parent) {}
				});
			}
		};
	}
}