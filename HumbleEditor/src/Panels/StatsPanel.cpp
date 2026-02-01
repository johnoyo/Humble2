#include "Systems\EditorPanelSystem.h"

namespace HBL2
{
	namespace Editor
	{
		void EditorPanelSystem::DrawStatsPanel(float ts)
		{
			const auto& appStats = Application::Get().GetStats();

			ImGui::Text("App");
			ImGui::NewLine();
			ImGui::Text("Frame Time: %f ms", ts * 1000.0f);

			ImGui::Text("Game Thread Time: %f ms", appStats.GameThreadTime);
			ImGui::Text("	Debug Draw Time: %f ms", appStats.DebugDrawTime);
			ImGui::Text("	App Update Time: %f ms", appStats.AppUpdateTime);
			ImGui::Text("	Gui Draw Time: %f ms", appStats.AppGuiDrawTime);
			ImGui::Text("	Game Thread Wait Time: %f ms", appStats.GameThreadWaitTime);

			ImGui::Text("Render Thread Time: %f ms", appStats.RenderThreadTime);
			ImGui::Text("	Render Thread Wait Time: %f ms", appStats.RenderThreadWaitTime);
			ImGui::Text("	Render Time: %f ms", appStats.RenderTime);
			ImGui::Text("	Present Time: %f ms", appStats.PresentTime);


			ImGui::Separator();

			const auto& stats = Renderer::Instance->GetStats();

			ImGui::Text("Renderer");
			ImGui::NewLine();
			ImGui::Text("Draw calls: %d", stats.DrawCalls);
			ImGui::Text("GatherTime: %f ms", stats.GatherTime);
			ImGui::Text("SortingTime: %f ms", stats.SortingTime);
			ImGui::Text("ShadowPass: %f ms", stats.ShadowPassTime);
			ImGui::Text("PrePass: %f ms", stats.PrePassTime);
			ImGui::Text("OpaquePass: %f ms", stats.OpaquePassTime);
			ImGui::Text("SkyboxPass: %f ms", stats.SkyboxPassTime);
			ImGui::Text("TransparentPass: %f ms", stats.TransparentPassTime);
			ImGui::Text("PostProcessPass: %f ms", stats.PostProcessPassTime);
			ImGui::Text("DebugPassTime: %f ms", stats.DebugPassTime);
			ImGui::Text("PresentPass: %f ms", stats.PresentPassTime);

			ImGui::Separator();

			ImGui::Text("Scene");
			ImGui::NewLine();
			ImGui::Text("Entities: %d", m_ActiveScene != nullptr ? m_ActiveScene->GetEntityCount() : 0);
			ImGui::NewLine();

			ImGui::Text("Systems");
			Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);
			if (activeScene != nullptr)
			{
				for (ISystem* system : activeScene->GetSystems())
				{
					if (system == nullptr)
					{
						continue;
					}

					ImGui::Text("%s: %f ms", system->Name.c_str(), system->RunningTime);
				}
			}

			ImGui::Separator();

			ImGui::Text("Arena Allocators");
			ImGui::Text("Arena: %f %%", Allocator::Arena.GetFullPercentage());
			ImGui::Text("Frame: %f %%", Allocator::Frame.GetFullPercentage());
			ImGui::Text("Persistent: %f %%", Allocator::Persistent.GetFullPercentage());
		}
	}
}