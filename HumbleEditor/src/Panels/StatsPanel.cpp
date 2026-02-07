#include "Systems\EditorPanelSystem.h"

namespace HBL2
{
	namespace Editor
	{
		static void TextWithCapacityColor(const char* label, int used, int capacity)
		{
			float ratio = capacity > 0 ? (float)used / (float)capacity : 0.0f;

			ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // white
			if (ratio > 0.90f)
			{
				color = ImVec4(1.0f, 0.25f, 0.25f, 1.0f); // red
			}
			else if (ratio > 0.75f)
			{
				color = ImVec4(1.0f, 1.0f, 0.25f, 1.0f); // yellow
			}

			ImGui::Text("%s:", label);
			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::Text("%d / %d", used, capacity);
			ImGui::PopStyleColor();
		}

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

			const auto& stats = Renderer::Instance->GetStatsForDisplay();

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

			ImGui::Text("Resource Manager");
			ImGui::NewLine();

			const auto& rmSpec = ResourceManager::Instance->GetSpec();
			const auto& rmStats = ResourceManager::Instance->GetUsageStats();

			TextWithCapacityColor("Textures Pool", rmStats.Textures, rmSpec.Textures);
			TextWithCapacityColor("Shaders Pool", rmStats.Shaders, rmSpec.Shaders);
			TextWithCapacityColor("Buffers Pool", rmStats.Buffers, rmSpec.Buffers);
			TextWithCapacityColor("BindGroups Pool", rmStats.BindGroups, rmSpec.BindGroups);
			TextWithCapacityColor("BindGroupLayouts Pool", rmStats.BindGroupLayouts, rmSpec.BindGroupLayouts);
			TextWithCapacityColor("FrameBuffers Pool", rmStats.FrameBuffers, rmSpec.FrameBuffers);
			TextWithCapacityColor("RenderPass Pool", rmStats.RenderPass, rmSpec.RenderPass);
			TextWithCapacityColor("RenderPassLayouts Pool", rmStats.RenderPassLayouts, rmSpec.RenderPassLayouts);
			TextWithCapacityColor("Meshes Pool", rmStats.Meshes, rmSpec.Meshes);
			TextWithCapacityColor("Materials Pool", rmStats.Materials, rmSpec.Materials);
			TextWithCapacityColor("Scenes Pool", rmStats.Scenes, rmSpec.Scenes);
			TextWithCapacityColor("Scripts Pool", rmStats.Scripts, rmSpec.Scripts);
			TextWithCapacityColor("Sounds Pool", rmStats.Sounds, rmSpec.Sounds);
			TextWithCapacityColor("Prefabs Pool", rmStats.Prefabs, rmSpec.Prefabs);

			ImGui::Separator();

			ImGui::Text("Asset Manager");
			ImGui::NewLine();

			const auto& amSpec = AssetManager::Instance->GetSpec();
			const auto& amStats = AssetManager::Instance->GetUsageStats();

			TextWithCapacityColor("Assets Pool", amStats.Assets, amSpec.Assets);

			ImGui::Separator();

			ImGui::Text("Arena Allocators");
			ImGui::NewLine();
			ImGui::Text("Arena: %f %%", Allocator::Arena.GetFullPercentage());
			ImGui::Text("Persistent: %f %%", Allocator::Persistent.GetFullPercentage());
		}
	}
}