#pragma once

#include "Core/Window.h"

#include <imgui.h>
#include <ImGuizmo.h>

namespace HBL2
{
	class HBL2_API ImGuiRenderer
	{
	public:
		static ImGuiRenderer* Instance;

		virtual void Initialize() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void Clean() = 0;

		// Wrapped ImGuizmo functions to work from dll.
		bool Gizmos_IsUsing();
		bool Gizmos_IsUsingViewManipulate();
		void Gizmos_SetOrthographic(bool isOrthographic);
		void Gizmos_SetDrawlist(ImDrawList* drawlist = nullptr);
		void Gizmos_SetRect(float x, float y, float width, float height);
		bool Gizmos_Manipulate(const float* view, const float* projection, ImGuizmo::OPERATION operation, ImGuizmo::MODE mode, float* matrix, float* deltaMatrix = NULL, const float* snap = NULL, const float* localBounds = NULL, const float* boundsSnap = NULL);
		void Gizmos_ViewManipulate(float* view, float length, ImVec2 position, ImVec2 size, ImU32 backgroundColor);

		ImGuiContext* GetContext();

	protected:
		Window* m_Window = nullptr;
		ImGuiContext* m_ImGuiContext = nullptr;
		const char* m_GlslVersion = nullptr;
	};
}