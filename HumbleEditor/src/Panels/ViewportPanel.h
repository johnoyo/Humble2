#pragma once

#include "EditorPanel.h"
#include "ImGui/ImGuiRenderer.h"

#include <glm/glm.hpp>

namespace HBL2::Editor
{
	class ViewportPanel final : public EditorPanel
	{
	public:
		ViewportPanel(const std::string& name, EditorPanelSystem* owner);

		virtual void OnAttach() override;
		virtual void OnCreate() override;
		virtual void OnOpen() override;
		virtual void OnRender(float ts) override;
		virtual void OnClose() override;
		virtual void OnDestroy() override;

	private:
		ImGuizmo::OPERATION m_GizmoOperation = ImGuizmo::OPERATION::BOUNDS;
		float m_CameraPivotDistance = 5.0f;
	};
}
