#include "TopBarPanel.h"

#include "Script/BuildEngine.h"
#include "Utilities/FileDialogs.h"
#include "Physics/PhysicsEngine2D.h"
#include "ImGui/ImGuiRenderer.h"
#include "Systems/EditorPanelSystem.h"

#include "EditorSettingsPanel.h"
#include "ProjectSettingsPanel.h"

namespace HBL2::Editor
{
	TopBarPanel::TopBarPanel(const std::string& name, EditorPanelSystem* owner)
	{
		m_Owner = owner;
		Name = name;
	}

	void TopBarPanel::OnAttach()
	{
	}

	void TopBarPanel::OnCreate()
	{
	}

	void TopBarPanel::OnOpen()
	{
	}

	void TopBarPanel::OnRender(float ts)
	{
		Scene* activeScene = m_Owner->m_ActiveScene;
		auto* editorAssetManager = (EditorAssetManager*)AssetManager::Instance;

		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Project"))
				{
					const std::string& filepath = HBL2::FileDialogs::SaveFile("Humble Project", Project::GetProjectDirectory().parent_path().string(), { "Humble Project Files (*.hblproj)", "*.hblproj" });

					if (!filepath.empty())
					{
						const std::string& projectName = std::filesystem::path(filepath).filename().stem().string();

						// Clean up registered systems.
						for (ISystem* system : activeScene->GetSystems())
						{
							system->OnDestroy();
						}

						// Unload all registered assets.
						editorAssetManager->DeregisterAssets();

						// Free unity build dll.
						BuildEngine::Instance->UnloadBuild(activeScene);

						// Create and open new project
						HBL2::Project::Create(projectName)->Save(filepath);

						editorAssetManager->CreateAsset({
							.debugName = "Empty Scene",
							.filePath = HBL2::Project::GetActive()->GetSpecification().StartingScene,
							.type = AssetType::Scene,
						});

						HBL2::Project::OpenStartingScene();

						m_Owner->m_ProjectChanged = true;
						m_Owner->m_EditorScenePath = HBL2::Project::GetAssetFileSystemPath(HBL2::Project::GetActive()->GetSpecification().StartingScene);
						m_Owner->m_CurrentDirectory = HBL2::Project::GetAssetDirectory();
					}
				}
				else if (ImGui::MenuItem("Open Project"))
				{
					const std::string& filepath = HBL2::FileDialogs::OpenFile("Humble Project", Project::GetProjectDirectory().parent_path().string(), { "Humble Project Files (*.hblproj)", "*.hblproj" });

					if (!filepath.empty())
					{
						for (ISystem* system : activeScene->GetSystems())
						{
							system->OnDestroy();
						}

						// Unload all registered assets.
						editorAssetManager->DeregisterAssets();

						// Free unity build dll.
						BuildEngine::Instance->UnloadBuild(activeScene);

						if (HBL2::Project::Load(std::filesystem::path(filepath)) != nullptr)
						{
							HBL2::Project::OpenStartingScene();

							m_Owner->m_ProjectChanged = true;
							m_Owner->m_EditorScenePath = HBL2::Project::GetAssetFileSystemPath(HBL2::Project::GetActive()->GetSpecification().StartingScene);
							m_Owner->m_CurrentDirectory = HBL2::Project::GetAssetDirectory();
						}
						else
						{
							HBL2_ERROR("Could not open specified project at path \"{0}\".", filepath);
						}
					}
				}
				else if (ImGui::MenuItem("Save Scene"))
				{
					if (m_Owner->m_EditorScenePath.empty())
					{
						std::string filepath = HBL2::FileDialogs::SaveFile("Humble Scene", Project::GetAssetDirectory().string(), { "Humble Scene Files (*.humble)", "*.humble" });
						auto relativePath = std::filesystem::relative(std::filesystem::path(filepath), HBL2::Project::GetAssetDirectory());

                        UUID sceneUUID = editorAssetManager->GetUUIDFromPath(relativePath);
						editorAssetManager->SaveAsset(sceneUUID);

                        m_Owner->m_EditorScenePath = filepath;
                    }
                    else
                    {
                        auto relativePath = std::filesystem::relative(m_Owner->m_EditorScenePath, HBL2::Project::GetAssetDirectory());

                        UUID sceneUUID = editorAssetManager->GetUUIDFromPath(relativePath);
						editorAssetManager->SaveAsset(sceneUUID);
					}
				}
				else if (ImGui::MenuItem("Save Scene As"))
				{
					std::string filepath = HBL2::FileDialogs::SaveFile("Humble Scene", Project::GetAssetDirectory().string(), { "Humble Scene Files (*.humble)", "*.humble" });
					auto relativePath = std::filesystem::relative(std::filesystem::path(filepath), HBL2::Project::GetAssetDirectory());

					auto assetHandle = editorAssetManager->CreateAsset({
						.debugName = strdup(relativePath.filename().stem().string().c_str()),
						.filePath = relativePath,
						.type = AssetType::Scene,
					});

					editorAssetManager->SaveAsset(assetHandle);
					Asset* asset = AssetManager::Instance->GetAssetMetadata(assetHandle);

					Handle<Scene> sceneHandle = Handle<Scene>::UnPack(asset->Indentifier);

					Scene* newScene = ResourceManager::Instance->GetScene(sceneHandle);
					Scene::Copy(activeScene, newScene);
					editorAssetManager->SaveAsset(assetHandle);

					SceneManager::Get().LoadScene(assetHandle, false);

					m_Owner->m_EditorScenePath = filepath;
				}
				else if (ImGui::MenuItem("New Scene"))
				{
					std::string filepath = HBL2::FileDialogs::SaveFile("Humble Scene", Project::GetAssetDirectory().string(), { "Humble Scene Files (*.humble)", "*.humble" });

					if (!filepath.empty())
					{
						auto relativePath = std::filesystem::relative(std::filesystem::path(filepath), HBL2::Project::GetAssetDirectory());

						auto assetHandle = editorAssetManager->CreateAsset({
							.debugName = "New Scene",
							.filePath = relativePath,
							.type = AssetType::Scene,
						});

						editorAssetManager->SaveAsset(assetHandle);

						if (Context::Mode == Mode::Runtime)
						{
							HBL2_WARN("Can not open a scene right now, exit play mode and then open scenes.");
						}
						else
						{
							HBL2::SceneManager::Get().LoadScene(assetHandle, false);
							m_Owner->m_EditorScenePath = filepath;
						}
					}
				}
				else if (ImGui::MenuItem("Open Scene"))
				{
					if (Context::Mode == Mode::Runtime)
					{
						HBL2_WARN("Can not open a scene right now, exit play mode and then open scenes.");
					}
					else
					{
						std::string filepath = HBL2::FileDialogs::OpenFile("Humble Scene", Project::GetAssetDirectory().string(), { "Humble Scene Files (*.humble)", "*.humble" });

						auto relativePath = std::filesystem::relative(std::filesystem::path(filepath), HBL2::Project::GetAssetDirectory());
                        UUID sceneUUID = editorAssetManager->GetUUIDFromPath(relativePath);
						HBL2::SceneManager::Get().LoadScene(AssetManager::Instance->GetHandleFromUUID(sceneUUID), false);

						m_Owner->m_EditorScenePath = filepath;
					}
				}
				else if (ImGui::MenuItem("Build (Windows - Debug)"))
				{
					const std::string& projectName = HBL2::Project::GetActive()->GetName();

					// Build.
					BuildEngine::Instance->BuildRuntime(BuildEngine::Configuration::Debug);

					// Copy project folder to build folder.
					FileUtils::CopyFolder("./" + projectName, "..\\bin\\Debug-x86_64\\HumbleApp\\" + projectName);

					// Copy assets folder to build folder.
					FileUtils::CopyFolder("./assets", "..\\bin\\Debug-x86_64\\HumbleApp\\assets");
				}
				else if (ImGui::MenuItem("Build (Windows - Release)"))
				{
					const std::string& projectName = HBL2::Project::GetActive()->GetName();

					// Build.
					BuildEngine::Instance->BuildRuntime(BuildEngine::Configuration::Release);

					// Copy project folder to build folder.
					FileUtils::CopyFolder("./" + projectName, "..\\bin\\Release-x86_64\\HumbleApp\\" + projectName);

					// Copy assets folder to build folder.
					FileUtils::CopyFolder("./assets", "..\\bin\\Release-x86_64\\HumbleApp\\assets");
				}
				else if (ImGui::MenuItem("Build (Windows - Distribution)"))
				{
					const std::string& projectName = HBL2::Project::GetActive()->GetName();

					// Build.
					BuildEngine::Instance->BuildRuntime(BuildEngine::Configuration::Distribution);

					// Copy project folder to build folder.
					FileUtils::CopyFolder("./" + projectName, "..\\bin\\Dist-x86_64\\HumbleApp\\" + projectName);

					// Copy assets folder to build folder.
					FileUtils::CopyFolder("./assets", "..\\bin\\Dist-x86_64\\HumbleApp\\assets");
				}
				else if (ImGui::MenuItem("Build & Run (Windows - Debug)"))
				{
					const std::string& projectName = HBL2::Project::GetActive()->GetName();

					// Build.
					BuildEngine::Instance->BuildRuntime(BuildEngine::Configuration::Debug);

					// Copy project folder to build folder.
					FileUtils::CopyFolder("./" + projectName, "..\\bin\\Debug-x86_64\\HumbleApp\\" + projectName);

					// Copy assets folder to build folder.
					FileUtils::CopyFolder("./assets", "..\\bin\\Debug-x86_64\\HumbleApp\\assets");

					// Run.
					BuildEngine::Instance->RunRuntime(BuildEngine::Configuration::Debug);
				}
				else if (ImGui::MenuItem("Build & Run (Windows - Release)"))
				{
					const std::string& projectName = HBL2::Project::GetActive()->GetName();

					// Build.
					BuildEngine::Instance->BuildRuntime(BuildEngine::Configuration::Release);

					// Copy project folder to build folder.
					FileUtils::CopyFolder("./" + projectName, "..\\bin\\Release-x86_64\\HumbleApp\\" + projectName);

					// Copy assets folder to build folder.
					FileUtils::CopyFolder("./assets", "..\\bin\\Release-x86_64\\HumbleApp\\assets");

					// Run.
					BuildEngine::Instance->RunRuntime(BuildEngine::Configuration::Release);
				}
				else if (ImGui::MenuItem("Build & Run (Windows - Distribution)"))
				{
					const std::string& projectName = HBL2::Project::GetActive()->GetName();

					// Build.
					BuildEngine::Instance->BuildRuntime(BuildEngine::Configuration::Distribution);

					// Copy project folder to build folder.
					FileUtils::CopyFolder("./" + projectName, "..\\bin\\Dist-x86_64\\HumbleApp\\" + projectName);

					// Copy assets folder to build folder.
					FileUtils::CopyFolder("./assets", "..\\bin\\Dist-x86_64\\HumbleApp\\assets");

					// Run.
					BuildEngine::Instance->RunRuntime(BuildEngine::Configuration::Release);
				}
				else if (ImGui::MenuItem("Build (Web)"))
				{
					system("..\\Scripts\\emBuildAll.bat");
				}
				else if (ImGui::MenuItem("Close"))
				{
					HBL2::Window::Instance->Close();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				for (EditorPanel* editorPanel : m_Owner->m_EditorPanels)
				{
					if (dynamic_cast<ProjectSettingsPanel*>(editorPanel))
					{
						if (ImGui::MenuItem("Open Project Settings"))
						{
							editorPanel->Enabled = true;
						}
					}
					
					if (dynamic_cast<EditorSettingsPanel*>(editorPanel))
					{
						if (ImGui::MenuItem("Open Editor Settings"))
						{
							editorPanel->Enabled = true;
						}
					}
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				for (EditorPanel* editorPanel : m_Owner->m_EditorPanels)
				{
					ImGui::Checkbox(editorPanel->Name.c_str(), &editorPanel->Enabled);
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
	}

	void TopBarPanel::OnClose()
	{
	}

	void TopBarPanel::OnDestroy()
	{
	}
}
