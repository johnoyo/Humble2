#pragma once

#include "EditorInspector.h"

#include "UI\Panel.h"
#include "UI\Elements.h"
#include "Scene\Components.h"

namespace HBL2::Editor
{
	class LinkEditor : public EditorInspector<LinkEditor, HBL2::Component::Link>
	{
	public:
		void OnCreate()
		{
			HBL2_CORE_INFO("Link Editor OnCreate");
			m_RenderBaseEditor = true;
		}

		void OnUpdate(const HBL2::Component::Link& component)
		{
			HBL2::UI::Panel
			{
				"Properties",
				HBL2::UI::Config
				{
					.id = "CustomLink",
					.mode = HBL2::UI::Rectagle{.color = { 0, 255, 0, 255 }, .cornerRadius = 8.0f },
					.layout = HBL2::UI::Layout{
						.sizing = {
							.width = HBL2::UI::Width::Grow(),
							.height = HBL2::UI::Height::Fixed(40)
						},
						.padding = { 4, 4 }
					}
				},
				[&](HBL2::UI::Panel* parent)
				{
					HBL2::UI::Text(parent, "Custom Link Editor!");
				}
			};
		}
	};
}