#include "ImGuiRenderer.h"

namespace HBL2
{
	ImGuiRenderer* ImGuiRenderer::Instance = nullptr;

	ImGuiContext* ImGuiRenderer::GetContext()
	{
		return m_ImGuiContext;
	}

	void ImGuiRenderer::SetImGuiStyle()
	{
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        // Background
        colors[ImGuiCol_WindowBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.0f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.05f, 0.05f, 0.05f, 1.0f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);

        // Borders
        colors[ImGuiCol_Border] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

        // Header (CollapsingHeader, Menus)
        colors[ImGuiCol_Header] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);

        // Buttons
        colors[ImGuiCol_Button] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);

        // Frame Background
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);

        // Scrollbars
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 1.0f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);

        // Tabs
        colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
        colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.07f, 0.07f, 1.0f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);

        // Text
        colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);

        // Adjust rounding and spacing
        style.FrameRounding = 4.0f;
        style.GrabRounding = 4.0f;
        style.WindowRounding = 0.0f;
        style.PopupRounding = 4.0f;
        style.TabRounding = 4.0f;
        style.ScrollbarRounding = 4.0f;
        style.WindowPadding = ImVec2(10.0f, 10.0f);
        style.FramePadding = ImVec2(5.0f, 5.0f);

        SetEditorTheme(ImVec4(0.850f, 0.600f, 0.150f, 1.0f));
	}

    void ImGuiRenderer::SetEditorTheme(const ImVec4& accentColor)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.Colors[ImGuiCol_ButtonHovered] = accentColor;
        style.Colors[ImGuiCol_FrameBgActive] = accentColor;
        style.Colors[ImGuiCol_SliderGrab] = accentColor;
        style.Colors[ImGuiCol_SliderGrabActive] = accentColor;
        style.Colors[ImGuiCol_CheckMark] = accentColor;
    }

	bool ImGuiRenderer::Gizmos_IsUsing()
	{
		return ImGuizmo::IsUsing();
	}

	bool ImGuiRenderer::Gizmos_IsUsingViewManipulate()
	{
		return ImGuizmo::IsUsingViewManipulate();
	}

	void ImGuiRenderer::Gizmos_SetOrthographic(bool isOrthographic)
	{
		ImGuizmo::SetOrthographic(isOrthographic);
	}

	void ImGuiRenderer::Gizmos_SetDrawlist(ImDrawList* drawlist)
	{
		ImGuizmo::SetDrawlist(drawlist);
	}

	void ImGuiRenderer::Gizmos_SetRect(float x, float y, float width, float height)
	{
		ImGuizmo::SetRect(x, y, width, height);
	}

	bool ImGuiRenderer::Gizmos_Manipulate(const float* view, const float* projection, ImGuizmo::OPERATION operation, ImGuizmo::MODE mode, float* matrix, float* deltaMatrix, const float* snap, const float* localBounds, const float* boundsSnap)
	{
		return ImGuizmo::Manipulate(view, projection, operation, mode, matrix, deltaMatrix, snap, localBounds, boundsSnap);
	}

	void ImGuiRenderer::Gizmos_ViewManipulate(float* view, float length, ImVec2 position, ImVec2 size, ImU32 backgroundColor)
	{
		ImGuizmo::ViewManipulate(view, length, position, size, backgroundColor);
	}
}