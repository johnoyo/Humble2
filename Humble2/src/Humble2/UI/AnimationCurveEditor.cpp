#include "AnimationCurveEditor.h"

#include "Systems/AnimationCurveSystem.h"

#include <imgui_internal.h>

namespace HBL2
{
	void AnimationCurveEditor::ShowCurveEditor(HBL2::Component::AnimationCurve& curve, const char* label)
	{
		static const char* presetNames[] = { "Linear", "Quad In", "Quad Out", "Cubic In", "Cubic Out", "Custom" };

		int curr = static_cast<int>(curve.Preset);
		if (ImGui::BeginCombo("Preset", presetNames[curr]))
		{
			for (int i = 0; i < IM_ARRAYSIZE(presetNames); ++i)
			{
				bool sel = (curr == i);
				if (ImGui::Selectable(presetNames[i], sel))
				{
					curve.Preset = static_cast<HBL2::Component::AnimationCurve::CurvePreset>(i);
				}

				if (sel) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		ImDrawList* draw = ImGui::GetWindowDrawList();
		const ImVec2 p0 = ImGui::GetCursorScreenPos();
		const ImVec2 sz = ImGui::GetContentRegionAvail();
		const ImVec2 p1 = { p0.x + sz.x, p0.y + sz.y };

		// grid
		const int GRID = 5;
		for (int i = 0; i <= GRID; ++i)
		{
			float t = i / float(GRID);
			float x = p0.x + sz.x * t;
			float y = p0.y + sz.y * (1.0f - t);

			draw->AddLine({ x, p0.y }, { x, p1.y }, IM_COL32(90, 90, 90, 255));
			draw->AddLine({ p0.x, y }, { p1.x, y }, IM_COL32(90, 90, 90, 255));

			char buf[8];
			ImFormatString(buf, 8, "%.1f", t);
			draw->AddText({ x - 6, p1.y + 2 }, IM_COL32(200, 200, 200, 255), buf);
			draw->AddText({ p0.x - 24, y - 6 }, IM_COL32(200, 200, 200, 255), buf);
		}

		// curve
		constexpr float CURVE_THICKNESS = 3.5f;
		const int SAMPLES = 128;

		ImVec2 prev{};
		for (int i = 0; i <= SAMPLES; ++i)
		{
			float t = i / float(SAMPLES);
			float val = AnimationCurveSystem::Evaluate(curve, t);

			ImVec2 pt = { p0.x + sz.x * t, p0.y + sz.y * (1.0f - val) };

			if (i > 0)
			{
				draw->AddLine(prev, pt, IM_COL32(230, 230, 100, 255), CURVE_THICKNESS);
			}

			prev = pt;
		}

		// key handles
		ImGuiIO& io = ImGui::GetIO();
		for (std::size_t i = 0; i < curve.Keys.size(); ++i)
		{
			HBL2::Component::AnimationCurve::KeyFrame& k = curve.Keys[i];
			ImVec2 pt = { p0.x + sz.x * k.Time, p0.y + sz.y * (1.0f - k.Value) };

			ImGui::SetCursorScreenPos({ pt.x - 5.0f, pt.y - 5.0f });
			ImGui::PushID(static_cast<int>(i));
			ImGui::InvisibleButton("kp", ImVec2(10, 10));

			bool active = ImGui::IsItemActive();

			if (active)
			{
				// mouse -> curve-space
				k.Time = std::clamp((io.MousePos.x - p0.x) / sz.x, 0.0f, 1.0f);
				k.Value = std::clamp(1.0f - (io.MousePos.y - p0.y) / sz.y, 0.0f, 1.0f);

				std::sort(curve.Keys.begin(), curve.Keys.end(), [](const HBL2::Component::AnimationCurve::KeyFrame& a, const HBL2::Component::AnimationCurve::KeyFrame& b)
				{
					return a.Time < b.Time;
				});

				curve.RecalculateTangents = true;
			}

			ImU32 col = (ImGui::IsItemHovered() || active) ? IM_COL32(250, 150, 150, 255) : IM_COL32(180, 100, 100, 255);
			draw->AddCircleFilled(pt, 4.0f, col);
			ImGui::PopID();
		}

		// double-click -> add key
		if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			float t = (io.MousePos.x - p0.x) / sz.x;
			t = std::clamp(t, 0.0f, 1.0f);

			// place the key exactly on the current curve so no kink appears
			float v = AnimationCurveSystem::Evaluate(curve, t);

			curve.Keys.push_back({ t, v });
			std::sort(curve.Keys.begin(), curve.Keys.end(), [](const HBL2::Component::AnimationCurve::KeyFrame& a, const HBL2::Component::AnimationCurve::KeyFrame& b)
			{
				return a.Time < b.Time;
			});

			curve.RecalculateTangents = true;
		}
	}
}