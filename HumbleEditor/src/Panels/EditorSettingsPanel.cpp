#include "Systems\EditorPanelSystem.h"

namespace HBL2
{
	namespace Editor
	{
		void EditorPanelSystem::DrawEditorSettingsPanel()
		{
			m_Context->GetRegistry()
				.view<Component::EditorCamera>()
				.each([&](entt::entity entity, Component::EditorCamera& editorCamera)
				{
					ImGui::Text("Camera Transform:");

					auto& transform = m_Context->GetComponent<HBL2::Component::Transform>(entity);

					ImGui::DragFloat3("Translation", glm::value_ptr(transform.Translation), 0.25f);
					ImGui::DragFloat3("Rotation", glm::value_ptr(transform.Rotation), 0.25f);
					ImGui::DragFloat3("Scale", glm::value_ptr(transform.Scale), 0.25f);

					ImGui::Separator();

					ImGui::Text("Camera View:");

					auto& camera = m_Context->GetComponent<HBL2::Component::Camera>(entity);

					ImGui::SliderFloat("Near", &camera.Near, 0, 10);
					ImGui::SliderFloat("Far", &camera.Far, 100, 1500);
					ImGui::SliderFloat("FOV", &camera.Fov, 0, 120);
					ImGui::SliderFloat("Aspect Ratio", &camera.AspectRatio, 0, 3);
					ImGui::SliderFloat("Exposure", &camera.Exposure, 0, 50);
					ImGui::SliderFloat("Gamma", &camera.Gamma, 0, 4);
					ImGui::SliderFloat("Zoom Level", &camera.ZoomLevel, 0, 500);					
				});
		}
	}
}