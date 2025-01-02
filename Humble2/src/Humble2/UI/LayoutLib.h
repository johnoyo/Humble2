#pragma once

#include "Base.h"
#include "Utilities\Collections\Span.h"

#include "glm\glm.hpp"

#include <initializer_list>

/*

class MenuSystem : public ISystem
{
	void OnGuiRender() override
	{
		UserInteface(Configuration
		{
			.ID = "OuterContainer",
			.mode = Rectagle
			{
				.color = { 43, 41, 51, 255 },
			},
			.layout = Layout
			{
				.direction = LAYOUT_DIRECTION::TOP_TO_BOTTOM,
				.sizing = {
					.width = 1920.0f,
					.height = 1080.0f,
				},
				.padding = { 16, 16 }
			}
		}, Body
		{
			{
				UserInteface(Configuration
				{
					.ID = "HeaderBar",
					.mode = Rectagle
					{
						.color = { 90, 90, 90, 255 },
						.cornerRadius = 8,
					}
					.layout = Layout
					{
						.sizing = {
							.width = 1920.0f,
							.height = 200.0f,
						}
					}
				},
			}
		})
	}
}
*/

#include "Panel.h"

#include "Text.h"

#include "UserInterfaceUtilities.h"