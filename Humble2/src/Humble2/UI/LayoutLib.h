#pragma once

#include "Base.h"
#include "Utilities\Collections\Span.h"

#include "glm\glm.hpp"

#include <initializer_list>

#include "Panel.h"

#include "Text.h"
#include "Image.h"

#include "Editor.h"

#include "UserInterfaceUtilities.h"

/*

	Examples of custom editors.

*/

class LinkEditor : public Editor<LinkEditor, HBL2::Component::Link>
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

class CameraEditor : public Editor<CameraEditor, HBL2::Component::Camera>
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