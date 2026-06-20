#include "TrayPanel.h"

#include "Core\Context.h"
#include "Core\Events.h"

#include "Scene\ISystem.h"
#include "Resources\ResourceManager.h"
#include "Script\BuildEngine.h"
#include "ImGui\ImGuiRenderer.h"
#include "Systems\EditorPanelSystem.h"

namespace HBL2::Editor
{
	TrayPanel::TrayPanel(const std::string& name, EditorPanelSystem* owner)
	{
		m_Owner = owner;
		Name = name;
	}

	void TrayPanel::OnAttach()
	{
		EventDispatcher::Get().Register<SceneChangeEvent>([this](const HBL2::SceneChangeEvent& e)
		{
			HBL2_CORE_INFO("TrayPanel::OnAttach::SceneChangeEvent");

			m_UserSystemNames.clear();
			m_UserComponentNames.clear();

			// Delete temporary play mode scene.
			Scene* currentScene = ResourceManager::Instance->GetScene(e.OldScene);
			if (currentScene != nullptr && currentScene->GetName().find("(Clone)") != StaticString<64>::npos)
			{
				if (m_HotReloadedDLL)
				{
					// Store all registered meta types.
					Reflect::ForEachRegisteredType([&](const Reflect::TypeEntry& entry)
					{
						const std::string& cleanName = BuildEngine::Instance->CleanComponentNameO3(std::string(entry.typeName));
						m_UserComponentNames.push_back(cleanName);
					});

					for (ISystem* userSystem : currentScene->GetRuntimeSystems())
					{
						if (userSystem->GetType() == SystemType::User)
						{
							m_UserSystemNames.push_back(userSystem->Name);
						}
					}
				}
			}
		});
	}

	void TrayPanel::OnCreate()
	{
		EventDispatcher::Get().Register<SceneChangeEvent>([this](const HBL2::SceneChangeEvent& e)
		{
			HBL2_CORE_INFO("TrayPanel::OnCreate::SceneChangeEvent");

			if (m_HotReloadedDLL)
			{
				BuildEngine::Instance->HotReload(e.NewScene, m_UserComponentNames, m_UserSystemNames, m_SerializedUserComponents);
				m_HotReloadedDLL = false;
			}
		});
	}

	void TrayPanel::OnOpen()
	{
	}

	void TrayPanel::OnRender(float ts)
	{
		ImGui::Begin(Name.c_str(), & m_CloseState, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);

		BuildEngine::Configuration config = BuildEngine::Instance->GetActiveConfiguration();
		std::string configButtonText = (config == BuildEngine::Configuration::Release) ? "Switch to Debug" : "Switch to Release";

		if (ImGui::Button(configButtonText.c_str()))
		{
			config = (config == BuildEngine::Configuration::Release) ? BuildEngine::Configuration::Debug : BuildEngine::Configuration::Release;
			BuildEngine::Instance->SetActiveConfiguration(config);
		}

		ImGui::SameLine();

		if (ImGui::Button("Recompile Scripts"))
		{
			// Flow when a HotReload happens:
			// 
			// [Event: Enter Play Mode]
			//		- Clone scene and load clone as the currently active scene.
			// [Event: Hot Reload]
			//		- Serialize User Components of Original scene.
			//		- Serialize User Components of Cloned scene.
			//		- Clear Reflection Registry.
			//		- Unload script DLL.
			//		- Build the script DLL.
			//		- Register Systems to Cloned scene.
			//			- Set their state to Play.
			//		- Register User Components (to Reflection).
			//		- Deserialize User Components to Cloned scene.
			// [Event: Exit Play Mode]
			//		- Cache User Component and User System names.
			//		- Clear and delete Cloned scene.
			//		- Clear User Components and Reflection Registry.
			//		- Deregister Systems.
			//		- Unload scripts DLL.
			//		- Load scripts DLL.
			//		- Register Systems to Original scene.
			//			- Set their state to Play.
			//		- Register User Components (to Reflection).
			//		- Deserialize User Components to Original scene.

			if (Context::Mode == Mode::Runtime)
			{
				HBL2_WARN("Hot reloading is experimental still, use with your own responsibility.");

				// Retrieve original scene, before entering play mode.
				Scene* originalScene = ResourceManager::Instance->GetScene(m_Owner->m_ActiveSceneTemp);

				// Only want to serialize the user components the first time we have a runtime recompilation.
				if (originalScene != nullptr && !m_HotReloadedDLL)
				{
					m_SerializedUserComponents.clear();

					// Store all registered meta types.
					Reflect::ForEachRegisteredType([&](const Reflect::TypeEntry& entry)
					{
						if (entry.serialize)
						{
							entry.serialize(&originalScene->GetRegistry(), m_SerializedUserComponents, true);
						}
					});
				}

				m_HotReloadedDLL = true;
			}
			else
			{
				m_HotReloadedDLL = false;
			}

			BuildEngine::Instance->Recompile();
		}

		ImGui::End();
	}

	void TrayPanel::OnClose()
	{
	}

	void TrayPanel::OnDestroy()
	{
	}
}