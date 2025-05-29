#include "Systems\EditorPanelSystem.h"

namespace HBL2
{
	namespace Editor
	{
		void EditorPanelSystem::DrawStatsPanel(float ts)
		{
			const auto& stats = Renderer::Instance->GetStats();

			ImGui::Text("Renderer");
			ImGui::NewLine();
			ImGui::Text("Frame Time: %f ms", ts * 1000.0f);
			ImGui::Text("Draw calls: %d", stats.DrawCalls);
			ImGui::Text("GatherTime: %f ms", stats.GatherTime);
			ImGui::Text("SortingTime: %f ms", stats.SortingTime);
			ImGui::Text("ShadowPass: %f ms", stats.ShadowPassTime);
			ImGui::Text("PrePass: %f ms", stats.PrePassTime);
			ImGui::Text("OpaquePass: %f ms", stats.OpaquePassTime);
			ImGui::Text("SkyboxPass: %f ms", stats.SkyboxPassTime);
			ImGui::Text("TransparentPass: %f ms", stats.TransparentPassTime);
			ImGui::Text("PostProcessPass: %f ms", stats.PostProcessPassTime);
			ImGui::Text("PresentPass: %f ms", stats.PresentPassTime);

			ImGui::Separator();

			ImGui::Text("Scene");
			ImGui::NewLine();
			ImGui::Text("Entities: %d", m_ActiveScene != nullptr ? m_ActiveScene->GetEntityCount() : 0);

			ImGui::Separator();

			ImGui::Text("Arena Allocators");
			ImGui::Text("Frame: %f %%", Allocator::Frame.GetFullPercentage());
			ImGui::Text("Scene: %f %%", Allocator::Scene.GetFullPercentage());
			ImGui::Text("App: %f %%", Allocator::App.GetFullPercentage());
		}
	}
}