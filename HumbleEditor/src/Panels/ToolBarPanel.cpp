#include "Systems\EditorPanelSystem.h"

#include "Script\BuildEngine.h"
#include "Utilities\FileDialogs.h"
#include "Physics\PhysicsEngine2D.h"

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
						const std::string& filepath = HBL2::FileDialogs::SaveFile("Humble Project", Project::GetProjectDirectory().parent_path().string(), { "Humble Project Files (*.hblproj)", "*.hblproj" });

						if (!filepath.empty())
						{
							const std::string& projectName = std::filesystem::path(filepath).filename().stem().string();

							// Clean up registered systems.
							for (ISystem* system : m_ActiveScene->GetSystems())
							{
								system->OnDestroy();
							}

							// Unload all registered assets.
							AssetManager::Instance->DeregisterAssets();

							// Free unity build dll.
							BuildEngine::Instance->UnloadBuild(m_ActiveScene);

							// Clear the invalid cached mesh handles, since we deregistered all the assets.
							MeshUtilities::Get().ClearCachedHandles();

							// Create and open new project
							HBL2::Project::Create(projectName)->Save(filepath);

							auto assetHandle = AssetManager::Instance->CreateAsset({
								.debugName = "Empty Scene",
								.filePath = HBL2::Project::GetActive()->GetSpecification().StartingScene,
								.type = AssetType::Scene,
							});

							MeshUtilities::Get().LoadBuiltInMeshes();
							HBL2::Project::OpenStartingScene();

							m_ProjectChanged = true;
							m_EditorScenePath = HBL2::Project::GetAssetFileSystemPath(HBL2::Project::GetActive()->GetSpecification().StartingScene);
							m_CurrentDirectory = HBL2::Project::GetAssetDirectory();
						}
					}
					else if (ImGui::MenuItem("Open Project"))
					{
						const std::string& filepath = HBL2::FileDialogs::OpenFile("Humble Project", Project::GetProjectDirectory().parent_path().string(), {"Humble Project Files (*.hblproj)", "*.hblproj"});

						if (!filepath.empty())
						{
							for (ISystem* system : m_ActiveScene->GetSystems())
							{
								system->OnDestroy();
							}

							// Unload all registered assets.
							AssetManager::Instance->DeregisterAssets();

							// Free unity build dll.
							BuildEngine::Instance->UnloadBuild(m_ActiveScene);

							// Clear the invalid cached mesh handles, since we deregistered all the assets.
							MeshUtilities::Get().ClearCachedHandles();

							if (HBL2::Project::Load(std::filesystem::path(filepath)) != nullptr)
							{
								MeshUtilities::Get().LoadBuiltInMeshes();
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
					if (ImGui::MenuItem("Project Settings"))
					{
						m_ShowProjectSettingsWindow = true;
					}

					if (ImGui::MenuItem("Editor Settings"))
					{
						m_ShowEditorSettingsWindow = true;
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("View"))
				{
					m_Context->View<Component::EditorPanel>()
						.Each([&](Component::EditorPanel& panel)
						{
							ImGui::Checkbox(panel.Name.c_str(), &panel.Enabled);
						});

					ImGui::EndMenu();
				}

				ImGui::EndMainMenuBar();
			}

			// Project Settings Window.
			if (m_ShowProjectSettingsWindow)
			{
				ImGui::Begin("Project Settings##Window", &m_ShowProjectSettingsWindow);

				auto& spec = HBL2::Project::GetActive()->GetSpecification();

				ImGui::Text("Renderer Settings");

				// Renderer.
				{
					{
						const char* options[] = { "Forward", "ForwardPlus", "Deffered", "Custom" };
						int currentItem = (int)spec.Settings.Renderer;

						if (ImGui::Combo("Renderer Type", &currentItem, options, IM_ARRAYSIZE(options)))
						{
							spec.Settings.Renderer = (RendererType)currentItem;
						}

						ImGui::SameLine();

						ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");
					}

					{
						const char* options[] = { "None", "OpenGL", "Vulkan" };
						int currentItem = (int)spec.Settings.EditorGraphicsAPI;

						if (ImGui::Combo("Editor Graphics API", &currentItem, options, IM_ARRAYSIZE(options)))
						{
							spec.Settings.EditorGraphicsAPI = (GraphicsAPI)currentItem;
						}

						ImGui::SameLine();

						ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");
					}

					{
						const char* options[] = { "None", "OpenGL", "Vulkan" };
						int currentItem = (int)spec.Settings.RuntimeGraphicsAPI;

						if (ImGui::Combo("Runtime Graphics API", &currentItem, options, IM_ARRAYSIZE(options)))
						{
							spec.Settings.RuntimeGraphicsAPI = (GraphicsAPI)currentItem;
						}
					}
				}

				ImGui::Separator();

				ImGui::Text("Physics2D Settings");

				// Physics 2d.
				{
					if (ImGui::DragFloat("Gravity##2d", &spec.Settings.GravityForce2D, 0.01f))
					{
					}

					if (ImGui::Checkbox("Enable Debug Draw##2d", &spec.Settings.EnableDebugDraw2D))
					{
						if (PhysicsEngine2D::Instance != nullptr)
						{
							PhysicsEngine2D::Instance->SetDebugDrawEnabled(spec.Settings.EnableDebugDraw2D);
						}
					}

					const char* options[] = { "Custom", "Box2D" };
					int currentItem = (int)spec.Settings.Physics2DImpl;

					if (ImGui::Combo("Implementation##2d", &currentItem, options, IM_ARRAYSIZE(options)))
					{
						spec.Settings.Physics2DImpl = (Physics2DEngineImpl)currentItem;
					}

					ImGui::SameLine();

					ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");
				}

				ImGui::Separator();

				ImGui::Text("Physics3D Settings");

				// Physics 3d.
				{
					if (ImGui::DragFloat("Gravity##3d", &spec.Settings.GravityForce3D, 0.01f))
					{
					}

					if (ImGui::Checkbox("Enable Debug Draw##3d", &spec.Settings.EnableDebugDraw3D))
					{
						if (PhysicsEngine3D::Instance != nullptr)
						{
							PhysicsEngine3D::Instance->SetDebugDrawEnabled(spec.Settings.EnableDebugDraw3D);
						}
					}

					if (spec.Settings.EnableDebugDraw3D)
					{
						if (ImGui::Checkbox("Show Colliders", &spec.Settings.ShowColliders3D))
						{
							if (PhysicsEngine3D::Instance != nullptr)
							{
								PhysicsEngine3D::Instance->ShowColliders(spec.Settings.ShowColliders3D);
							}
						}

						if (ImGui::Checkbox("Show Bounding Boxes", &spec.Settings.ShowBoundingBoxes3D))
						{
							if (PhysicsEngine3D::Instance != nullptr)
							{
								PhysicsEngine3D::Instance->ShowBoundingBoxes(spec.Settings.ShowBoundingBoxes3D);
							}
						}
					}

					const char* options[] = { "Custom", "Jolt" };
					int currentItem = (int)spec.Settings.Physics3DImpl;

					if (ImGui::Combo("Implementation##3d", &currentItem, options, IM_ARRAYSIZE(options)))
					{
						spec.Settings.Physics3DImpl = (Physics3DEngineImpl)currentItem;
					}

					ImGui::SameLine();

					ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");
				}

				ImGui::Separator();

				ImGui::Text("Editor Settings");

				{
					if (ImGui::Checkbox("Multiple Viewports", &spec.Settings.EditorMultipleViewports))
					{
					}

					ImGui::SameLine();

					ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");
				}

				ImGui::Separator();

				ImGui::Text("Audio Settings");

				{
					const char* options[] = { "Custom", "FMOD" };
					int currentItem = (int)spec.Settings.SoundImpl;

					if (ImGui::Combo("Implementation##sound", &currentItem, options, IM_ARRAYSIZE(options)))
					{
						spec.Settings.SoundImpl = (SoundEngineImpl)currentItem;
					}

					ImGui::SameLine();

					ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");
				}

				ImGui::Separator();

				ImGui::Text("Advanced Settings");

				{
					if (ImGui::InputScalar("Max App Memory", ImGuiDataType_U32, (void*)(intptr_t*)&spec.Settings.MaxAppMemory))
					{
					}

					ImGui::SameLine();

					ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");

					if (ImGui::InputScalar("Max UniformBuffer Memory", ImGuiDataType_U32, (void*)(intptr_t*)&spec.Settings.MaxUniformBufferMemory))
					{
					}

					ImGui::SameLine();

					ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");
				}

				ImGui::Separator();

				if (ImGui::Button("Save Project Settings"))
				{
					HBL2::Project::GetActive()->Save();
				}

				ImGui::End();
			}
		
			if (m_ShowEditorSettingsWindow)
			{
				ImGui::Begin("Editor Settings##Window", &m_ShowEditorSettingsWindow);

				// Gizmo mode.
				{
					const char* options[] = { "Local", "World" };
					int currentItem = (int)m_GizmoMode;

					if (ImGui::Combo("GizmoMode", &currentItem, options, IM_ARRAYSIZE(options)))
					{
						m_GizmoMode = (ImGuizmo::MODE)currentItem;
					}
				}

				ImGui::Separator();

				m_Context->View<Component::EditorCamera>()
					.Each([&](Entity entity, Component::EditorCamera& editorCamera)
					{
						ImGui::Text("Camera Transform:");

						auto& transform = m_Context->GetComponent<HBL2::Component::Transform>(entity);

						ImGui::DragFloat3("Translation", glm::value_ptr(transform.Translation), 0.25f);
						ImGui::DragFloat3("Rotation", glm::value_ptr(transform.Rotation), 0.25f);
						ImGui::DragFloat3("Scale", glm::value_ptr(transform.Scale), 0.25f);

						ImGui::Separator();

						ImGui::Text("Camera View:");

						auto& camera = m_Context->GetComponent<HBL2::Component::Camera>(entity);

						ImGui::SliderFloat("Near", &camera.Near, 0, 10);
						ImGui::SliderFloat("Far", &camera.Far, 100, 2500);
						ImGui::SliderFloat("FOV", &camera.Fov, 0, 120);
						ImGui::SliderFloat("Aspect Ratio", &camera.AspectRatio, 0, 3);
						ImGui::SliderFloat("Exposure", &camera.Exposure, 0, 50);
						ImGui::SliderFloat("Gamma", &camera.Gamma, 0, 4);
						ImGui::SliderFloat("Zoom Level", &camera.ZoomLevel, 0, 500);

						ImGui::Separator();

						ImGui::Text("Camera Controls:");

						ImGui::SliderFloat("MovementSpeed", &editorCamera.MovementSpeed, 0, 150);
						ImGui::SliderFloat("MouseSensitivity", &editorCamera.MouseSensitivity, 0, 100);
						ImGui::SliderFloat("PanSpeed", &editorCamera.PanSpeed, 0, 200);
						ImGui::SliderFloat("ZoomSpeed", &editorCamera.ZoomSpeed, 0, 15);
						ImGui::SliderFloat("ScrollZoomSpeed", &editorCamera.ScrollZoomSpeed, 0, 200);
					});

				ImGui::End();
			}
		}
	}
}