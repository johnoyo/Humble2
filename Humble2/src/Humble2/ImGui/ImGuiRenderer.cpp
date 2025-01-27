#include "ImGuiRenderer.h"

namespace HBL2
{
	ImGuiRenderer* ImGuiRenderer::Instance = nullptr;

	ImGuiContext* ImGuiRenderer::GetContext()
	{
		return m_ImGuiContext;
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