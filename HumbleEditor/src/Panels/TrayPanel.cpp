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
						for (auto meta_type : entt::resolve(originalScene->GetMetaContext()))
						{
							const auto& alias = meta_type.second.info().name();

							if (alias.size() == 0 || alias.size() >= UINT32_MAX || alias.data() == nullptr)
							{
								HBL2_CORE_ERROR("Empty meta type registered on scene {}!", originalScene->GetName());
								continue;
							}

							const std::string& cleanName = BuildEngine::Instance->CleanComponentNameO3(alias.data());
							BuildEngine::Instance->SerializeComponents(cleanName, originalScene, m_SerializedUserComponents);
						}
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