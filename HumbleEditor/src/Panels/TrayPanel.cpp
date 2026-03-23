#include "Systems\EditorPanelSystem.h"

#include "Script\BuildEngine.h"

namespace HBL2
{
	namespace Editor
	{
		void EditorPanelSystem::DrawTrayPanel()
		{
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
					Scene* originalScene = ResourceManager::Instance->GetScene(m_ActiveSceneTemp);

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
		}
	}
}