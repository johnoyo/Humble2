#include "Systems\EditorPanelSystem.h"

namespace HBL2
{
	namespace Editor
	{
		void EditorPanelSystem::DrawStatsPanel(float ts)
		{
			const auto& stats = Renderer::Instance->GetRendererStats();

			ImGui::Text("Renderer");
			ImGui::NewLine();
			ImGui::Text("Frame Time: %f ms", ts);
			ImGui::Text("Draw calls: %d", stats.DrawCalls);

			ImGui::Separator();

			ImGui::Text("Scene");
			ImGui::NewLine();
			ImGui::Text("Entities: %d", m_ActiveScene != nullptr ? m_ActiveScene->GetEntityCount() : 0);
		}
	}
}