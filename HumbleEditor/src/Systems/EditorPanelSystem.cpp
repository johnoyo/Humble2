#include "EditorPanelSystem.h"

#include "Humble2\Utilities\FileDialogs.h"
#include "Humble2\Utilities\YamlUtilities.h"
#include "EditorCameraSystem.h"

#include "Resources\Handle.h"
#include "Resources\ResourceManager.h"
#include "Resources\Types.h"
#include "Resources\TypeDescriptors.h"

#include "UI/LayoutLib.h"
#include "Utilities/UnityBuild.h"

namespace HBL2
{
	namespace Editor
	{
		void EditorPanelSystem::OnCreate()
		{
			m_ActiveScene = ResourceManager::Instance->GetScene(Context::ActiveScene);
			m_EditorScenePath = HBL2::Project::GetAssetFileSystemPath(HBL2::Project::GetActive()->GetSpecification().StartingScene);

			EventDispatcher::Get().Register("SceneChangeEvent", [&](const HBL2::Event& e)
			{
				HBL2_CORE_INFO("EditorPanelSystem::SceneChangeEvent");
				const SceneChangeEvent& sce = dynamic_cast<const SceneChangeEvent&>(e);

				// Delete temporary play mode scene.
				Scene* currentScene = ResourceManager::Instance->GetScene(sce.OldScene);
				if (currentScene != nullptr && currentScene->GetName().find("(Clone)") != std::string::npos)
				{
					// Clear entire scene
					currentScene->Clear();

					// Unload unity build dll.
					NativeScriptUtilities::Get().UnloadUnityBuild(currentScene);

					// Delete play mode scene.
					ResourceManager::Instance->DeleteScene(sce.OldScene);
				}

				m_ActiveScene = ResourceManager::Instance->GetScene(sce.NewScene);

				// Clear selected entity
				HBL2::Component::EditorVisible::SelectedEntity = entt::null;

				if (Context::Mode == Mode::Runtime)
				{
					for (ISystem* system : m_ActiveScene->GetRuntimeSystems())
					{
						system->SetState(SystemState::Play);
					}
				}
				else if (Context::Mode == Mode::Editor)
				{
					for (ISystem* system : m_ActiveScene->GetRuntimeSystems())
					{
						system->SetState(SystemState::Idle);
					}
				}
			});

			{
				// Hierachy panel.
				auto hierachyPanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(hierachyPanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(hierachyPanel);
				panel.Name = "Hierachy";
				panel.Type = Component::EditorPanel::Panel::Hierachy;
				panel.Render = [this]() { DrawHierachyPanel(); };
			}

			{
				// Properties panel.
				auto propertiesPanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(propertiesPanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(propertiesPanel);
				panel.Name = "Properties";
				panel.Type = Component::EditorPanel::Panel::Properties;
				panel.Render = [this]() { DrawPropertiesPanel(); };
			}

			{
				// Menubar panel.
				auto menubarPanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(menubarPanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(menubarPanel);
				panel.Name = "Menubar";
				panel.Type = Component::EditorPanel::Panel::Menubar;
				panel.UseBeginEnd = false;
				panel.Render = [this]() { DrawToolBarPanel(); };
			}

			{
				// Stats panel.
				auto statsPanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(statsPanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(statsPanel);
				panel.Name = "Stats";
				panel.Type = Component::EditorPanel::Panel::Stats;
				panel.Render = [this]() { DrawStatsPanel(Time::DeltaTime); };
			}

			{
				// Content browser panel.
				auto contentBrowserPanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(contentBrowserPanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(contentBrowserPanel);
				panel.Name = "Content Browser";
				panel.Type = Component::EditorPanel::Panel::ContentBrowser;
				m_CurrentDirectory = HBL2::Project::GetAssetDirectory();
				panel.Render = [this]() { DrawContentBrowserPanel(); };
			}

			{
				// Console panel.
				auto consolePanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(consolePanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(consolePanel);
				panel.Name = "Console";
				panel.Type = Component::EditorPanel::Panel::Console;
				panel.Render = [this]() { DrawConsolePanel(Time::DeltaTime); };
			}

			{
				// Viewport panel.
				auto viewportPanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(viewportPanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(viewportPanel);
				panel.Name = "Viewport";
				panel.Type = Component::EditorPanel::Panel::Viewport;
				panel.Styles.push_back({ ImGuiStyleVar_WindowPadding, ImVec2{ 0.f, 0.f }, 0.f, false });
				panel.Render = [this]() { DrawViewportPanel(); };
			}

			{
				// Play / Stop panel.
				auto playStopPanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(playStopPanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(playStopPanel);
				panel.Name = "Play / Stop";
				panel.Type = Component::EditorPanel::Panel::PlayStop;
				panel.Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
				panel.Render = [this]() { DrawPlayStopPanel(); };
			}

			{
				// Systems panel.
				auto systems = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(systems).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(systems);
				panel.Name = "Systems";
				panel.Type = Component::EditorPanel::Panel::Systems;
				panel.Render = [this]() { DrawSystemsPanel(); };
			}

			{
				// Bottom tray panel.
				auto bottomTrayPanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(bottomTrayPanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(bottomTrayPanel);
				panel.Name = "Bottom Tray";
				panel.Type = Component::EditorPanel::Panel::Tray;
				panel.Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
				panel.Render = [this]() { DrawTrayPanel(); };
			}

			// TODO: Remove from here!

			HBL2::EditorUtilities::Get().RegisterCustomEditor<HBL2::Component::Link, LinkEditor>();
			HBL2::EditorUtilities::Get().InitCustomEditor<HBL2::Component::Link, LinkEditor>();

			HBL2::EditorUtilities::Get().RegisterCustomEditor<HBL2::Component::Camera, CameraEditor>();
			HBL2::EditorUtilities::Get().InitCustomEditor<HBL2::Component::Camera, CameraEditor>();
		}

		void EditorPanelSystem::OnUpdate(float ts)
		{
		}

		void EditorPanelSystem::OnGuiRender(float ts)
		{
			m_Context->GetRegistry()
				.view<Component::EditorPanel>()
				.each([&](Component::EditorPanel& panel)
				{
					if (panel.Enabled)
					{
						// Push style vars for this window.
						for (auto& style : panel.Styles)
						{
							if (style.UseFloat)
							{
								ImGui::PushStyleVar(style.StyleVar, style.FloatValue);
							}
							else
							{
								ImGui::PushStyleVar(style.StyleVar, style.VectorValue);
							}
						}

						// Push window to the stack.
						if (panel.UseBeginEnd)
						{
							ImGui::Begin(panel.Name.c_str(), &panel.Closeable, panel.Flags);
						}

						panel.Render();

						if (m_ProjectChanged)
						{
							m_ProjectChanged = false;
							return;
						}

						// Pop window from the stack.
						if (panel.UseBeginEnd)
						{
							ImGui::End();
						}

						// Pop style vars.
						for (auto& style : panel.Styles)
						{
							ImGui::PopStyleVar();
						}
					}
				});
		}

		void EditorPanelSystem::OnDestroy()
		{
		}
	}
}