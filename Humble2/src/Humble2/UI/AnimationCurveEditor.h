#pragma once

#include "Panel.h"
#include "EditorInspector.h"

#include "Scene\Components.h"
#include "Elements.h"

namespace HBL2
{
	class HBL2_API AnimationCurveEditor : public EditorInspector<AnimationCurveEditor, HBL2::Component::AnimationCurve>
	{
	public:
		void OnCreate()
		{
			HBL2_CORE_INFO("AnimationCurve Editor OnCreate");
			m_RenderBaseEditor = true;
		}

		void OnUpdate(const HBL2::Component::AnimationCurve& curve)
		{
			HBL2::UI::CreateEditorPanel("Properties",
				HBL2::UI::Config
				{
					.id = "CustomAnimationCurve",
					.mode = HBL2::UI::Rectagle{ .color = { 0, 0, 0, 0 }, .cornerRadius = 8.0f },
					.layout = HBL2::UI::Layout{
						.sizing = {
							.width = HBL2::UI::Width::Grow(),
							.height = HBL2::UI::Height::Grow(),
						},
						.padding = { 4, 4 }
					}
				},
				[&](HBL2::UI::Panel* parent)
				{
					ShowCurveEditor(*(HBL2::Component::AnimationCurve*)&curve, "Curve Editor");
				}
			);
		}

	private:
		void ShowCurveEditor(HBL2::Component::AnimationCurve& curve, const char* label);
	};
}