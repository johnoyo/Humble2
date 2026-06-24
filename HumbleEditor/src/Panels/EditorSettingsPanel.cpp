#include "EditorSettingsPanel.h"

#include "ImGui/ImGuiRenderer.h"
#include "Systems/EditorPanelSystem.h"

namespace HBL2::Editor
{
	EditorSettingsPanel::EditorSettingsPanel(const std::string& name, EditorPanelSystem* owner)
	{
		m_Owner = owner;
		Name = name;
		Enabled = false;
	}

	void EditorSettingsPanel::OnAttach()
	{
	}

	void EditorSettingsPanel::OnCreate()
	{
	}

	void EditorSettingsPanel::OnOpen()
	{
	}

	void EditorSettingsPanel::OnRender(float ts)
	{
		Scene* ctx = m_Owner->m_Context;

		ImGui::Begin(Name.c_str(), &m_CloseState);

		// Gizmo mode.
		{
			const char* options[] = { "Local", "World" };
			int currentItem = (int)m_Owner->m_GizmoMode;

			if (ImGui::Combo("GizmoMode", &currentItem, options, IM_ARRAYSIZE(options)))
			{
				m_Owner->m_GizmoMode = (ImGuizmo::MODE)currentItem;
			}
		}

		ImGui::Separator();

		ctx->Filter<Component::EditorCamera>()
			.ForEach([&](Entity entity, Component::EditorCamera& editorCamera)
			{
				ImGui::Text("Camera Transform:");

				auto& transform = ctx->GetComponent<HBL2::Component::Transform>(entity);

				ImGui::DragFloat3("Translation", glm::value_ptr(transform.Translation), 0.25f);
				ImGui::DragFloat3("Rotation", glm::value_ptr(transform.Rotation), 0.25f);
				ImGui::DragFloat3("Scale", glm::value_ptr(transform.Scale), 0.25f);

				ImGui::Separator();

				ImGui::Text("Camera View:");

				auto& camera = ctx->GetComponent<HBL2::Component::Camera>(entity);

				ImGui::SliderFloat("Near", &camera.Near, 0, 10);
				ImGui::SliderFloat("Far", &camera.Far, 100, 2500);
				ImGui::SliderFloat("FOV", &camera.Fov, 0, 120);
				ImGui::SliderFloat("Aspect Ratio", &camera.AspectRatio, 0, 3);
				ImGui::SliderFloat("Exposure", &camera.Exposure, 0, 50);
				ImGui::SliderFloat("Gamma", &camera.Gamma, 0, 4);
				ImGui::SliderFloat("Zoom Level", &camera.ZoomLevel, 0, 500);

				ImGui::Separator();

				ImGui::Text("Camera Controls:");

				ImGui::SliderFloat("MovementSpeed", &editorCamera.MovementSpeed, 0, 150);
				ImGui::SliderFloat("MouseSensitivity", &editorCamera.MouseSensitivity, 0, 100);
				ImGui::SliderFloat("PanSpeed", &editorCamera.PanSpeed, 0, 200);
				ImGui::SliderFloat("ZoomSpeed", &editorCamera.ZoomSpeed, 0, 15);
				ImGui::SliderFloat("ScrollZoomSpeed", &editorCamera.ScrollZoomSpeed, 0, 200);
			});

		ImGui::End();
	}

	void EditorSettingsPanel::OnClose()
	{
	}

	void EditorSettingsPanel::OnDestroy()
	{
	}
}
