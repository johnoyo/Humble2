#include "Systems\EditorPanelSystem.h"

#include <Utilities\FileDialogs.h>

namespace HBL2
{
	namespace Editor
	{
		void EditorPanelSystem::DrawToolBarPanel()
		{
			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("New Project"))
					{
						std::string filepath = HBL2::FileDialogs::SaveFile("Humble Project", Project::GetProjectDirectory().parent_path().string(), { "Humble Project Files (*.hblproj)", "*.hblproj" });

						std::string projectName = std::filesystem::path(filepath).filename().stem().string();

						// Clean up registered systems.
						for (ISystem* system : m_ActiveScene->GetSystems())
						{
							system->OnDestroy();
						}

						// Unload all registered assets.
						AssetManager::Instance->DeregisterAssets();

						// Free unity build dll.
						NativeScriptUtilities::Get().UnloadUnityBuild(m_ActiveScene);

						// Clear the invalid cached mesh handles, since we deregistered all the assets.
						MeshUtilities::Get().ClearCachedHandles();

						// Create and open new project
						HBL2::Project::Create(projectName)->Save(filepath);

						auto assetHandle = AssetManager::Instance->CreateAsset({
							.debugName = "Empty Scene",
							.filePath = HBL2::Project::GetActive()->GetSpecification().StartingScene,
							.type = AssetType::Scene,
						});

						HBL2::Project::OpenStartingScene();

						m_ProjectChanged = true;
						m_EditorScenePath = HBL2::Project::GetAssetFileSystemPath(HBL2::Project::GetActive()->GetSpecification().StartingScene);
						m_CurrentDirectory = HBL2::Project::GetAssetDirectory();
					}
					else if (ImGui::MenuItem("Open Project"))
					{
						std::string filepath = HBL2::FileDialogs::OpenFile("Humble Project", Project::GetProjectDirectory().parent_path().string(), {"Humble Project Files (*.hblproj)", "*.hblproj"});

						for (ISystem* system : m_ActiveScene->GetSystems())
						{
							system->OnDestroy();
						}

						// Unload all registered assets.
						AssetManager::Instance->DeregisterAssets();

						// Free unity build dll.
						NativeScriptUtilities::Get().UnloadUnityBuild(m_ActiveScene);

						// Clear the invalid cached mesh handles, since we deregistered all the assets.
						MeshUtilities::Get().ClearCachedHandles();

						if (HBL2::Project::Load(std::filesystem::path(filepath)) != nullptr)
						{
							HBL2::Project::OpenStartingScene();

							m_ProjectChanged = true;
							m_EditorScenePath = HBL2::Project::GetAssetFileSystemPath(HBL2::Project::GetActive()->GetSpecification().StartingScene);
							m_CurrentDirectory = HBL2::Project::GetAssetDirectory();
						}
						else
						{
							HBL2_ERROR("Could not open specified project at path \"{0}\".", filepath);
						}
					}
					else if (ImGui::MenuItem("Save Scene"))
					{
						if (m_EditorScenePath.empty())
						{
							std::string filepath = HBL2::FileDialogs::SaveFile("Humble Scene", Project::GetAssetDirectory().string(), { "Humble Scene Files (*.humble)", "*.humble" });
							auto relativePath = std::filesystem::relative(std::filesystem::path(filepath), HBL2::Project::GetAssetDirectory());

							AssetManager::Instance->SaveAsset(std::hash<std::string>()(relativePath.string()));

							m_EditorScenePath = filepath;
						}
						else
						{
							auto relativePath = std::filesystem::relative(m_EditorScenePath, HBL2::Project::GetAssetDirectory());

							AssetManager::Instance->SaveAsset(std::hash<std::string>()(relativePath.string()));
						}
					}
					else if (ImGui::MenuItem("Save Scene As"))
					{
						std::string filepath = HBL2::FileDialogs::SaveFile("Humble Scene", Project::GetAssetDirectory().string(), { "Humble Scene Files (*.humble)", "*.humble" });
						auto relativePath = std::filesystem::relative(std::filesystem::path(filepath), HBL2::Project::GetAssetDirectory());

						auto assetHandle = AssetManager::Instance->CreateAsset({
							.debugName = _strdup(relativePath.filename().stem().string().c_str()),
							.filePath = relativePath,
							.type = AssetType::Scene,
						});

						AssetManager::Instance->SaveAsset(assetHandle);
						Asset* asset = AssetManager::Instance->GetAssetMetadata(assetHandle);

						Handle<Scene> sceneHandle = Handle<Scene>::UnPack(asset->Indentifier);

						Scene* newScene = ResourceManager::Instance->GetScene(sceneHandle);
						Scene::Copy(m_ActiveScene, newScene);
						AssetManager::Instance->SaveAsset(assetHandle);

						SceneManager::Get().LoadScene(assetHandle, false);

						m_EditorScenePath = filepath;
					}
					else if (ImGui::MenuItem("New Scene"))
					{
						std::string filepath = HBL2::FileDialogs::SaveFile("Humble Scene", Project::GetAssetDirectory().string(), { "Humble Scene Files (*.humble)", "*.humble" });
						auto relativePath = std::filesystem::relative(std::filesystem::path(filepath), HBL2::Project::GetAssetDirectory());

						auto assetHandle = AssetManager::Instance->CreateAsset({
							.debugName = "New Scene",
							.filePath = relativePath,
							.type = AssetType::Scene,
						});

						AssetManager::Instance->SaveAsset(assetHandle);

						HBL2::SceneManager::Get().LoadScene(assetHandle, false);

						m_EditorScenePath = filepath;
					}
					else if (ImGui::MenuItem("Open Scene"))
					{
						std::string filepath = HBL2::FileDialogs::OpenFile("Humble Scene", Project::GetAssetDirectory().string(), { "Humble Scene Files (*.humble)", "*.humble" });

						auto relativePath = std::filesystem::relative(std::filesystem::path(filepath), HBL2::Project::GetAssetDirectory());
						UUID sceneUUID = std::hash<std::string>()(relativePath.string());

						HBL2::SceneManager::Get().LoadScene(AssetManager::Instance->GetHandleFromUUID(sceneUUID), false);

						m_EditorScenePath = filepath;
					}
					else if (ImGui::MenuItem("Build (Windows - Debug)"))
					{
						const std::string& projectName = HBL2::Project::GetActive()->GetName();

						// Build.
						system("\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\msbuild.exe\" ..\\HumbleGameEngine2.sln /t:HumbleApp /p:Configuration=Debug");

						// Copy project folder to build folder.
						FileUtils::CopyFolder("./" + projectName, "..\\bin\\Debug-x86_64\\HumbleApp\\" + projectName);

						// Copy assets folder to build folder.
						FileUtils::CopyFolder("./assets", "..\\bin\\Debug-x86_64\\HumbleApp\\assets");
					}
					else if (ImGui::MenuItem("Build (Windows - Release)"))
					{
						const std::string& projectName = HBL2::Project::GetActive()->GetName();

						// Build.
						system("\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\msbuild.exe\" ..\\HumbleGameEngine2.sln /t:HumbleApp /p:Configuration=Release");

						// Copy project folder to build folder.
						FileUtils::CopyFolder("./" + projectName, "..\\bin\\Release-x86_64\\HumbleApp\\" + projectName);

						// Copy assets folder to build folder.
						FileUtils::CopyFolder("./assets", "..\\bin\\Release-x86_64\\HumbleApp\\assets");
					}
					else if (ImGui::MenuItem("Build & Run (Windows - Debug)"))
					{
						const std::string& projectName = HBL2::Project::GetActive()->GetName();

						// Build.
						system("\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\msbuild.exe\" ..\\HumbleGameEngine2.sln /t:HumbleApp /p:Configuration=Debug");

						// Copy project folder to build folder.
						FileUtils::CopyFolder("./" + projectName, "..\\bin\\Debug-x86_64\\HumbleApp\\" + projectName);

						// Copy assets folder to build folder.
						FileUtils::CopyFolder("./assets", "..\\bin\\Debug-x86_64\\HumbleApp\\assets");

						// Run.
						system("cd ..\\bin\\Debug-x86_64\\HumbleApp && HumbleApp.exe");
					}
					else if (ImGui::MenuItem("Build & Run (Windows - Release)"))
					{
						const std::string& projectName = HBL2::Project::GetActive()->GetName();

						// Build.
						system("\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\msbuild.exe\" ..\\HumbleGameEngine2.sln /t:HumbleApp /p:Configuration=Release");

						// Copy project folder to build folder.
						FileUtils::CopyFolder("./" + projectName, "..\\bin\\Release-x86_64\\HumbleApp\\" + projectName);

						// Copy assets folder to build folder.
						FileUtils::CopyFolder("./assets", "..\\bin\\Release-x86_64\\HumbleApp\\assets");

						// Run.
						system("cd ..\\bin\\Release-x86_64\\HumbleApp && HumbleApp.exe");
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
					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("View"))
				{
					m_Context->GetRegistry()
						.view<Component::EditorPanel>()
						.each([&](Component::EditorPanel& panel)
						{
							ImGui::Checkbox(panel.Name.c_str(), &panel.Enabled);
						});

					ImGui::EndMenu();
				}

				ImGui::EndMainMenuBar();
			}
		}
	}
}