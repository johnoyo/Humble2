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