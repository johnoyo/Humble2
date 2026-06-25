#include "EditorPanelSystem.h"

#include "Humble2/Utilities/FileDialogs.h"
#include "Humble2/Utilities/YamlUtilities.h"
#include "Humble2/Utilities/Collections/StaticString.h"
#include "EditorCameraSystem.h"

#include "Resources/Handle.h"
#include "Resources/ResourceManager.h"
#include "Resources/Types.h"
#include "Resources/TypeDescriptors.h"

#include "Inspectors/LinkEditor.h"
#include "Inspectors/CameraEditor.h"
#include "Inspectors/AnimationCurveEditor.h"

#include "Panels/PlayStopPanel.h"
#include "Panels/SystemsPanel.h"
#include "Panels/TrayPanel.h"
#include "Panels/StatsPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/ConsolePanel.h"
#include "Panels/TopBarPanel.h"
#include "Panels/EditorSettingsPanel.h"
#include "Panels/ProjectSettingsPanel.h"
#include "Panels/HierachyPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/PropertiesPanel.h"

namespace HBL2
{
	namespace Editor
	{
		void EditorPanelSystem::OnCreate()
		{
			m_ActiveScene = ResourceManager::Instance->GetScene(Context::ActiveScene);
			m_EditorScenePath = HBL2::Project::GetAssetFileSystemPath(HBL2::Project::GetActive()->GetSpecification().StartingScene);

			m_EditorPanels.push_back(new PlayStopPanel("Play / Stop", this));
			m_EditorPanels.push_back(new ViewportPanel("Viewport", this));
			m_EditorPanels.push_back(new HierachyPanel("Hierachy", this));
			m_EditorPanels.push_back(new SystemsPanel("Systems", this));
			m_EditorPanels.push_back(new StatsPanel("Stats", this));
			m_EditorPanels.push_back(new ContentBrowserPanel("Content Browser", this));
			m_EditorPanels.push_back(new PropertiesPanel("Properties", this));
			m_EditorPanels.push_back(new ConsolePanel("Console", this));
			m_EditorPanels.push_back(new TrayPanel("Bottom Tray", this));
			m_EditorPanels.push_back(new TopBarPanel("Menubar", this));
			m_EditorPanels.push_back(new EditorSettingsPanel("Editor Settings", this));
			m_EditorPanels.push_back(new ProjectSettingsPanel("Project Settings", this));

			for (EditorPanel* editorPanel : m_EditorPanels)
			{
				editorPanel->OnAttach();
			}

			EventDispatcher::Get().Register<SceneChangeEvent>([&](const HBL2::SceneChangeEvent& e)
			{
				HBL2_CORE_INFO("EditorPanelSystem::SceneChangeEvent");

				// Delete temporary play mode scene.
				Scene* currentScene = ResourceManager::Instance->GetScene(e.OldScene);
				if (currentScene != nullptr && currentScene->GetName().find("(Clone)") != StaticString<64>::npos)
				{
					// Clear entire scene
					currentScene->Clear();

					// Delete play mode scene.
					ResourceManager::Instance->DeleteScene(e.OldScene);
				}

				// Clear selected entity
				HBL2::Component::EditorVisible::SelectedEntity = Entity::Null;
				m_ActiveScene = ResourceManager::Instance->GetScene(e.NewScene);

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

				if (m_ActiveScene != nullptr)
				{
					m_ActiveScene->Filter<HBL2::Component::Camera>()
						.ForEach([&](HBL2::Component::Camera& camera)
						{
							if (camera.Enabled)
							{
								camera.AspectRatio = m_ViewportSize.x / m_ViewportSize.y;
							}
						});
				}
			});

			for (EditorPanel* editorPanel : m_EditorPanels)
			{
				editorPanel->OnCreate();
			}

			// TODO: Remove from here!

			RegisterCustomEditor<HBL2::Component::Link, LinkEditor>();
			InitCustomEditor<HBL2::Component::Link, LinkEditor>();

			RegisterCustomEditor<HBL2::Component::Camera, CameraEditor>();
			InitCustomEditor<HBL2::Component::Camera, CameraEditor>();

			RegisterCustomEditor<HBL2::Component::AnimationCurve, AnimationCurveEditor>();
			InitCustomEditor<HBL2::Component::AnimationCurve, AnimationCurveEditor>();
		}

		void EditorPanelSystem::OnUpdate(float ts)
		{
		}

		void EditorPanelSystem::OnGuiRender(float ts)
		{
			for (EditorPanel* editorPanel : m_EditorPanels)
			{
				if (editorPanel->GotEnabled())
				{
					editorPanel->OnOpen();
				}

				if (editorPanel->Enabled)
				{
					editorPanel->OnRender(ts);
				}

				if (m_ProjectChanged)
				{
					m_ProjectChanged = false;
					return;
				}

				if (editorPanel->GotDisabled())
				{
					editorPanel->OnClose();
				}
			}
		}

		void EditorPanelSystem::OnDestroy()
		{
			for (EditorPanel* editorPanel : m_EditorPanels)
			{
				editorPanel->OnDestroy();
			}
			m_EditorPanels.clear();

			m_CustomEditors.clear();
		}
	}
}
