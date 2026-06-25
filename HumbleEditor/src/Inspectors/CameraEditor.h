#pragma once

#include "EditorInspector.h"

#include "UI/Panel.h"
#include "UI/Elements.h"
#include "Scene/Components.h"

namespace HBL2::Editor
{
	class CameraEditor : public EditorInspector<CameraEditor, HBL2::Component::Camera>
	{
	public:
		void OnCreate()
		{
			HBL2_CORE_INFO("Camera Editor OnCreate");
			m_RenderBaseEditor = true;
		}

		void OnUpdate(const HBL2::Component::Camera& component)
		{
			HBL2::UI::Panel
			{
				"Properties",
				HBL2::UI::Config
				{
					.id = "CustomCamera",
					.mode = HBL2::UI::Rectagle{.color = { 255, 0, 0, 255 }, .cornerRadius = 8.0f },
					.layout = HBL2::UI::Layout{
						.sizing = {
							.width = 160,
							.height = 40
						},
						.padding = { 4, 4 }
					}
				},
				[&](HBL2::UI::Panel* parent)
				{
					HBL2::UI::Text(parent, "Custom Camera Editor!");
				}
			};
		}
	};
}
