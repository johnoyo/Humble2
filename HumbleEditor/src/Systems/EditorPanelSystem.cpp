#include "EditorPanelSystem.h"

#include "Humble2\Utilities\FileDialogs.h"
#include "Humble2\Utilities\YamlUtilities.h"
#include "EditorCameraSystem.h"

#include "Renderer\Rewrite\Handle.h"
#include "Renderer\Rewrite\ResourceManager.h"
#include "Renderer\Rewrite\Types.h"
#include "Renderer\Rewrite\TypeDescriptors.h"

namespace HBL2
{
	namespace Editor
	{
		void EditorPanelSystem::OnCreate()
		{
			// Load editor icons.
			/*m_PngIcon = HBL2::Texture::Load("assets/icons/content_browser/png-1477.png")->GetID();
			m_JpgIcon = HBL2::Texture::Load("assets/icons/content_browser/jpg-1476.png")->GetID();
			m_ObjIcon = HBL2::Texture::Load("assets/icons/content_browser/obj-1472.png")->GetID();
			m_FbxIcon = HBL2::Texture::Load("assets/icons/content_browser/fbx-1472.png")->GetID();
			m_MatIcon = HBL2::Texture::Load("assets/icons/content_browser/mat-1472.png")->GetID();
			m_MtlIcon = HBL2::Texture::Load("assets/icons/content_browser/mtl-1472.png")->GetID();
			m_SceneIcon = HBL2::Texture::Load("assets/icons/content_browser/scene-1472.png")->GetID();
			m_Mp3Icon = HBL2::Texture::Load("assets/icons/content_browser/mp3-1474.png")->GetID();
			m_TxtIcon = HBL2::Texture::Load("assets/icons/content_browser/txt-1473.png")->GetID();
			m_ShaderIcon = HBL2::Texture::Load("assets/icons/content_browser/shader-1472.png")->GetID();
			m_FileIcon = HBL2::Texture::Load("assets/icons/content_browser/file-1453.png")->GetID();
			m_FolderIcon = HBL2::Texture::Load("assets/icons/content_browser/folder-1437.png")->GetID();
			m_BackIcon = HBL2::Texture::Load("assets/icons/content_browser/curved-arrow-4608.png")->GetID();*/

			m_Context = HBL2::Context::ActiveScene;

			{
				// Hierachy panel.
				auto hierachyPanel = HBL2::Context::Core->CreateEntity();
				HBL2::Context::Core->GetComponent<HBL2::Component::Tag>(hierachyPanel).Name = "Hidden";
				auto& panel = HBL2::Context::Core->AddComponent<Component::EditorPanel>(hierachyPanel);
				panel.Name = "Hierachy";
				panel.Type = Component::EditorPanel::Panel::Hierachy;
			}

			{
				// Properties panel.
				auto propertiesPanel = HBL2::Context::Core->CreateEntity();
				HBL2::Context::Core->GetComponent<HBL2::Component::Tag>(propertiesPanel).Name = "Hidden";
				auto& panel = HBL2::Context::Core->AddComponent<Component::EditorPanel>(propertiesPanel);
				panel.Name = "Properties";
				panel.Type = Component::EditorPanel::Panel::Properties;
			}

			{
				// Menubar panel.
				auto menubarPanel = HBL2::Context::Core->CreateEntity();
				HBL2::Context::Core->GetComponent<HBL2::Component::Tag>(menubarPanel).Name = "Hidden";
				auto& panel = HBL2::Context::Core->AddComponent<Component::EditorPanel>(menubarPanel);
				panel.Name = "Menubar";
				panel.Type = Component::EditorPanel::Panel::Menubar;
				panel.UseBeginEnd = false;
			}

			{
				// Stats panel.
				auto statsPanel = HBL2::Context::Core->CreateEntity();
				HBL2::Context::Core->GetComponent<HBL2::Component::Tag>(statsPanel).Name = "Hidden";
				auto& panel = HBL2::Context::Core->AddComponent<Component::EditorPanel>(statsPanel);
				panel.Name = "Stats";
				panel.Type = Component::EditorPanel::Panel::Stats;
			}

			{
				// Content browser panel.
				auto contentBrowserPanel = HBL2::Context::Core->CreateEntity();
				HBL2::Context::Core->GetComponent<HBL2::Component::Tag>(contentBrowserPanel).Name = "Hidden";
				auto& panel = HBL2::Context::Core->AddComponent<Component::EditorPanel>(contentBrowserPanel);
				panel.Name = "Content Browser";
				panel.Type = Component::EditorPanel::Panel::ContentBrowser;
				m_CurrentDirectory = HBL2::Project::GetAssetDirectory();
			}

			{
				// Console panel.
				auto consolePanel = HBL2::Context::Core->CreateEntity();
				HBL2::Context::Core->GetComponent<HBL2::Component::Tag>(consolePanel).Name = "Hidden";
				auto& panel = HBL2::Context::Core->AddComponent<Component::EditorPanel>(consolePanel);
				panel.Name = "Console";
				panel.Type = Component::EditorPanel::Panel::Console;
			}

			{
				// Viewport panel.
				auto viewportPanel = HBL2::Context::Core->CreateEntity();
				HBL2::Context::Core->GetComponent<HBL2::Component::Tag>(viewportPanel).Name = "Hidden";
				auto& panel = HBL2::Context::Core->AddComponent<Component::EditorPanel>(viewportPanel);
				panel.Name = "Viewport";
				panel.Type = Component::EditorPanel::Panel::Viewport;
				panel.Styles.push_back({ ImGuiStyleVar_WindowPadding, ImVec2{ 0.f, 0.f }, 0.f, false });
			}

			// Create and register assets.
			for (auto& entry : std::filesystem::recursive_directory_iterator(HBL2::Project::GetAssetDirectory()))
			{
				const std::string& extension = entry.path().extension().string();

				if (extension == ".png" || extension == ".jpg")
				{
					auto assetHandle = AssetManager::Instance->CreateAsset({
						.debugName = "texture-asset",
						.filePath = entry.path(),
						.type = AssetType::Texture,
					});
				}
				else if (extension == ".obj" || extension == ".gltf" || extension == ".glb" || extension == ".fbx")
				{
					auto assetHandle = AssetManager::Instance->CreateAsset({
						.debugName = "mesh-asset",
						.filePath = entry.path(),
						.type = AssetType::Mesh,
					});
				}
				else if (extension == ".hblmat")
				{
					auto assetHandle = AssetManager::Instance->CreateAsset({
						.debugName = "material-asset",
						.filePath = entry.path(),
						.type = AssetType::Material,
					});
				}
				else if (extension == ".hblshader")
				{
					auto assetHandle = AssetManager::Instance->CreateAsset({
						.debugName = "shader-asset",
						.filePath = entry.path(),
						.type = AssetType::Shader,
					});
				}
				else if (extension == ".humble")
				{
					auto assetHandle = AssetManager::Instance->CreateAsset({
						.debugName = "scene-asset",
						.filePath = entry.path(),
						.type = AssetType::Scene,
					});
				}
			}
		}

		void EditorPanelSystem::OnUpdate(float ts)
		{
			m_Context = HBL2::Context::ActiveScene;
		}

		void EditorPanelSystem::OnGuiRender(float ts)
		{
			HBL2::Context::Core->GetRegistry()
				.view<Component::EditorPanel>()
				.each([&](Component::EditorPanel& panel)
				{
					if (panel.Enabled)
					{
						// Push style vars for this window.
						for (auto& style : panel.Styles)
						{
							if (style.UseFloat)
								ImGui::PushStyleVar(style.StyleVar, style.FloatValue);
							else
								ImGui::PushStyleVar(style.StyleVar, style.VectorValue);
						}

						// Push window to the stack.
						if (panel.UseBeginEnd)
							ImGui::Begin(panel.Name.c_str(), &panel.Closeable, panel.Flags);

						// Draw the appropriate window.
						switch (panel.Type)
						{
						case Component::EditorPanel::Panel::Hierachy:
							DrawHierachyPanel();
							break;
						case Component::EditorPanel::Panel::Properties:
							DrawPropertiesPanel();
							break;
						case Component::EditorPanel::Panel::Viewport:
							DrawViewportPanel();
							break;
						case Component::EditorPanel::Panel::Menubar:
							DrawToolBarPanel();
							break;
						case Component::EditorPanel::Panel::Stats:
							DrawStatsPanel(ts);
							break;
						case Component::EditorPanel::Panel::ContentBrowser:
							DrawContentBrowserPanel();
							break;
						case Component::EditorPanel::Panel::Console:
							DrawConsolePanel(ts);
							break;
						}

						// Pop window from the stack.
						if (panel.UseBeginEnd)
							ImGui::End();

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

		void EditorPanelSystem::DrawHierachyPanel()
		{
			// Pop up menu when right clicking on an empty space inside the hierachy panel.
			if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
			{
				if (ImGui::MenuItem("Create Empty"))
				{
					auto entity = m_Context->CreateEntity();
					m_Context->AddComponent<HBL2::Component::EditorVisible>(entity);
				}

				if (ImGui::MenuItem("Create Sprite"))
				{
					auto entity = m_Context->CreateEntity();
					m_Context->GetComponent<HBL2::Component::Tag>(entity).Name = "Sprite";
					m_Context->GetComponent<HBL2::Component::Transform>(entity).Translation = { 0.f, 15.f, 0.f };
					m_Context->AddComponent<HBL2::Component::EditorVisible>(entity);
					m_Context->AddComponent<HBL2::Component::Sprite>(entity);
				}

				if (ImGui::MenuItem("Create Camera"))
				{
					auto entity = m_Context->CreateEntity();
					m_Context->GetComponent<HBL2::Component::Tag>(entity).Name = "Camera";
					m_Context->AddComponent<HBL2::Component::EditorVisible>(entity);
					m_Context->AddComponent<HBL2::Component::Camera>(entity).Enabled = true;
					m_Context->GetComponent<HBL2::Component::Transform>(entity).Translation.z = 100.f;
				}

				if (ImGui::MenuItem("Create Monkeh"))
				{
					auto entity = m_Context->CreateEntity();
					m_Context->GetComponent<HBL2::Component::Tag>(entity).Name = "Monkeh";
					m_Context->GetComponent<HBL2::Component::Transform>(entity).Scale = { 5.f, 5.f, 5.f };
					m_Context->AddComponent<HBL2::Component::EditorVisible>(entity);
					auto& mesh = m_Context->AddComponent<HBL2::Component::StaticMesh>(entity);

					// TODO: Get active project.
					mesh.Path = "EmptyProject\\Assets\\Meshes\\monkey_smooth.obj";
					mesh.TexturePath = "";
					mesh.ShaderName = "BasicMesh";
				}

				if (ImGui::MenuItem("Create Lost Empire"))
				{
					auto entity = m_Context->CreateEntity();
					m_Context->GetComponent<HBL2::Component::Tag>(entity).Name = "LostEmpire";
					m_Context->GetComponent<HBL2::Component::Transform>(entity).Scale = { 5.f, 5.f, 5.f };
					m_Context->AddComponent<HBL2::Component::EditorVisible>(entity);
					auto& mesh = m_Context->AddComponent<HBL2::Component::StaticMesh>(entity);

					// TODO: Get active project.
					mesh.Path = "EmptyProject\\Assets\\Meshes\\lost_empire.obj";
					mesh.TexturePath = "EmptyProject\\Assets\\Textures\\lost_empire-RGBA.png";
					mesh.ShaderName = "BasicMesh";
				}

				ImGui::EndPopup();
			}

			entt::entity entityToBeDeleted = entt::null;

			// Iterate over all editor visible entities and draw the to the hierachy panel.
			m_Context->GetRegistry()
				.group<HBL2::Component::EditorVisible>(entt::get<HBL2::Component::Tag>)
				.each([&](entt::entity entity, HBL2::Component::EditorVisible& editorVisible, HBL2::Component::Tag& tag)
				{
					if (editorVisible.Enabled)
					{
						ImGuiTreeNodeFlags flags = ((editorVisible.Selected && editorVisible.SelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
						bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.Name.c_str());

						if (ImGui::IsItemClicked())
						{
							editorVisible.Selected = true;
							editorVisible.SelectedEntity = entity;
						}

						if (ImGui::BeginPopupContextItem())
						{
							if (ImGui::MenuItem("Destroy"))
							{
								// Defer the deletion at the end of the function, for now just mark the entity.
								entityToBeDeleted = entity;
							}

							ImGui::EndPopup();
						}

						if (opened)
						{
							ImGui::TreePop();
						}
					}
				});

			if (entityToBeDeleted != entt::null)
			{
				// Destroy entity and clear entityToBeDeleted value.
				m_Context->DestroyEntity(entityToBeDeleted);
				entityToBeDeleted = entt::null;

				// Clear currently selected entity.
				HBL2::Component::EditorVisible::SelectedEntity = entt::null;
				HBL2::Component::EditorVisible::Selected = false;
			}

			// Clear selection if clicked on empty space inside hierachy panel.
			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
			{
				HBL2::Component::EditorVisible::SelectedEntity = entt::null;
				HBL2::Component::EditorVisible::Selected = false;
			}
		}

		void EditorPanelSystem::DrawPropertiesPanel()
		{
			const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap;

			if (HBL2::Component::EditorVisible::SelectedEntity != entt::null)
			{
				// Tag component.
				if (m_Context->HasComponent<HBL2::Component::Tag>(HBL2::Component::EditorVisible::SelectedEntity))
				{
					auto& tag = m_Context->GetComponent<HBL2::Component::Tag>(HBL2::Component::EditorVisible::SelectedEntity).Name;

					char buffer[256];
					memset(buffer, 0, sizeof(buffer));
					strcpy_s(buffer, sizeof(buffer), tag.c_str());

					if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
					{
						tag = std::string(buffer);
					}

					ImGui::Separator();
				}

				// Transform component.
				if (m_Context->HasComponent<HBL2::Component::Transform>(HBL2::Component::EditorVisible::SelectedEntity))
				{
					if (ImGui::TreeNodeEx((void*)typeid(HBL2::Component::Transform).hash_code(), treeNodeFlags, "Transform"))
					{
						auto& transform = m_Context->GetComponent<HBL2::Component::Transform>(HBL2::Component::EditorVisible::SelectedEntity);

						ImGui::DragFloat3("Translation", glm::value_ptr(transform.Translation), 0.25f);
						ImGui::DragFloat3("Rotation", glm::value_ptr(transform.Rotation), 0.25f);
						ImGui::DragFloat3("Scale", glm::value_ptr(transform.Scale), 0.25f);

						ImGui::TreePop();
					}

					ImGui::Separator();
				}

				// Camera component.
				if (m_Context->HasComponent<HBL2::Component::Camera>(HBL2::Component::EditorVisible::SelectedEntity))
				{
					bool opened = ImGui::TreeNodeEx((void*)typeid(HBL2::Component::Camera).hash_code(), treeNodeFlags, "Camera");

					ImGui::SameLine(ImGui::GetWindowWidth() - 25.f);

					bool removeComponent = false;

					if (ImGui::Button("-", ImVec2{ 18.f, 18.f }))
					{
						removeComponent = true;
					}

					if (opened)
					{
						auto& camera = m_Context->GetComponent<HBL2::Component::Camera>(HBL2::Component::EditorVisible::SelectedEntity);

						ImGui::Checkbox("Enabled", &camera.Enabled);
						ImGui::Checkbox("Primary", &camera.Primary);
						ImGui::SliderFloat("Far", &camera.Far, 0, 100);
						ImGui::SliderFloat("Near", &camera.Near, 100, 1500);
						ImGui::SliderFloat("FOV", &camera.Fov, 0, 120);
						ImGui::SliderFloat("Aspect Ratio", &camera.AspectRatio, 0, 2);
						ImGui::SliderFloat("Zoom Level", &camera.ZoomLevel, 0, 500);

						ImGui::TreePop();
					}

					ImGui::Separator();

					if (removeComponent)
					{
						m_Context->RemoveComponent<HBL2::Component::Camera>(HBL2::Component::EditorVisible::SelectedEntity);
					}
				}

				// Sprite component.
				if (m_Context->HasComponent<HBL2::Component::Sprite>(HBL2::Component::EditorVisible::SelectedEntity))
				{
					bool opened = ImGui::TreeNodeEx((void*)typeid(HBL2::Component::Sprite).hash_code(), treeNodeFlags, "Sprite");

					ImGui::SameLine(ImGui::GetWindowWidth() - 25.f);

					bool removeComponent = false;

					if (ImGui::Button("-", ImVec2{ 18.f, 18.f }))
					{
						removeComponent = true;
					}

					if (opened)
					{
						auto& sprite = m_Context->GetComponent<HBL2::Component::Sprite>(HBL2::Component::EditorVisible::SelectedEntity);

						ImGui::Checkbox("Enabled", &sprite.Enabled);
						ImGui::InputText("Texture", (char*)sprite.Path.c_str(), 256);

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item"))
							{
								auto path = std::filesystem::relative(HBL2::Project::GetAssetFileSystemPath((const wchar_t*)payload->Data), HBL2::Project::GetProjectDirectory().parent_path());

								if (path.extension().string() == ".png" || path.extension().string() == ".jpg")
								{
									sprite.Path = path.string();
								}
								else
								{
									HBL2_WARN("Could not load {0} - not a valid texture format file", path.filename().string());
								}

								ImGui::EndDragDropTarget();
							}
						}

						ImGui::ColorEdit4("Color", glm::value_ptr(sprite.Color));

						ImGui::TreePop();
					}

					ImGui::Separator();

					if (removeComponent)
					{
						m_Context->RemoveComponent<HBL2::Component::Sprite>(HBL2::Component::EditorVisible::SelectedEntity);
					}
				}

				// StaticMesh_New component.
				if (m_Context->HasComponent<HBL2::Component::StaticMesh_New>(HBL2::Component::EditorVisible::SelectedEntity))
				{
					bool opened = ImGui::TreeNodeEx((void*)typeid(HBL2::Component::StaticMesh_New).hash_code(), treeNodeFlags, "Static Mesh New");

					ImGui::SameLine(ImGui::GetWindowWidth() - 25.f);

					bool removeComponent = false;

					if (ImGui::Button("-", ImVec2{ 18.f, 18.f }))
					{
						removeComponent = true;
					}

					if (opened)
					{
						auto& mesh = m_Context->GetComponent<HBL2::Component::StaticMesh_New>(HBL2::Component::EditorVisible::SelectedEntity);
						
						uint32_t meshHandle = mesh.Mesh.Pack();
						uint32_t materialHandle = mesh.Material.Pack();

						ImGui::Checkbox("Enabled", &mesh.Enabled);
						ImGui::InputScalar("Mesh", ImGuiDataType_U32, (void*)(intptr_t*)&meshHandle);

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Mesh"))
							{
								uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
								Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

								mesh.Mesh = AssetManager::Instance->GetAsset<Mesh>(assetHandle);

								ImGui::EndDragDropTarget();
							}
						}

						ImGui::InputScalar("Material", ImGuiDataType_U32, (void*)(intptr_t*)&materialHandle);

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Material"))
							{
								uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
								Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

								mesh.Material = AssetManager::Instance->GetAsset<Material>(assetHandle);

								ImGui::EndDragDropTarget();
							}
						}

						ImGui::TreePop();
					}

					ImGui::Separator();

					if (removeComponent)
					{
						m_Context->RemoveComponent<HBL2::Component::StaticMesh_New>(HBL2::Component::EditorVisible::SelectedEntity);
					}
				}

				// StaticMesh component.
				if (m_Context->HasComponent<HBL2::Component::StaticMesh>(HBL2::Component::EditorVisible::SelectedEntity))
				{
					bool opened = ImGui::TreeNodeEx((void*)typeid(HBL2::Component::StaticMesh).hash_code(), treeNodeFlags, "Static Mesh");

					ImGui::SameLine(ImGui::GetWindowWidth() - 25.f);

					bool removeComponent = false;

					if (ImGui::Button("-", ImVec2{ 18.f, 18.f }))
					{
						removeComponent = true;
					}

					if (opened)
					{
						auto& mesh = m_Context->GetComponent<HBL2::Component::StaticMesh>(HBL2::Component::EditorVisible::SelectedEntity);

						ImGui::Checkbox("Enabled", &mesh.Enabled);
						ImGui::InputText("Mesh", (char*)mesh.Path.c_str(), 256);

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item"))
							{
								auto path = std::filesystem::relative(HBL2::Project::GetAssetFileSystemPath((const wchar_t*)payload->Data), HBL2::Project::GetProjectDirectory().parent_path());

								if (path.extension().string() == ".obj")
								{
									// TODO: Update this to use new API.
									/*HBL2::Renderer3D::Get().CleanMesh(mesh);
									mesh.Path = path.string();
									HBL2::Renderer3D::Get().SubmitMesh(m_Context->GetComponent<HBL2::Component::Transform>(HBL2::Component::EditorVisible::SelectedEntity), mesh);*/
								}
								else
								{
									HBL2_WARN("Could not load {0} - not a valid mesh format file", path.filename().string());
								}

								ImGui::EndDragDropTarget();
							}
						}

						ImGui::InputText("Texture", (char*)mesh.TexturePath.c_str(), 256);

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item"))
							{
								auto path = std::filesystem::relative(HBL2::Project::GetAssetFileSystemPath((const wchar_t*)payload->Data), HBL2::Project::GetProjectDirectory().parent_path());

								if (path.extension().string() == ".png" || path.filename().extension().string() == ".jpg")
								{
									mesh.TexturePath = path.string();

									// TODO: Update this to use new API.
									/*HBL2::Renderer3D::Get().CleanMesh(mesh);
									HBL2::Renderer3D::Get().SubmitMesh(m_Context->GetComponent<HBL2::Component::Transform>(HBL2::Component::EditorVisible::SelectedEntity), mesh);*/
								}
								else
								{
									HBL2_WARN("Could not load {0} - not a valid texture format file", path.filename().string());
								}

								ImGui::EndDragDropTarget();
							}
						}

						ImGui::TreePop();
					}

					ImGui::Separator();

					if (removeComponent)
					{
						m_Context->RemoveComponent<HBL2::Component::StaticMesh>(HBL2::Component::EditorVisible::SelectedEntity);
					}
				}

				// Add component button.
				if (ImGui::Button("Add Component"))
				{
					ImGui::OpenPopup("AddComponent");
				}

				if (ImGui::BeginPopup("AddComponent"))
				{
					if (ImGui::MenuItem("Sprite"))
					{
						m_Context->AddComponent<HBL2::Component::Sprite>(HBL2::Component::EditorVisible::SelectedEntity);
						ImGui::CloseCurrentPopup();
					}

					if (ImGui::MenuItem("StaticMesh"))
					{
						m_Context->AddComponent<HBL2::Component::StaticMesh>(HBL2::Component::EditorVisible::SelectedEntity);
						ImGui::CloseCurrentPopup();
					}

					if (ImGui::MenuItem("Camera"))
					{
						m_Context->AddComponent<HBL2::Component::Camera>(HBL2::Component::EditorVisible::SelectedEntity);
						ImGui::CloseCurrentPopup();
					}

					ImGui::EndPopup();
				}
			}
		}

		void EditorPanelSystem::DrawToolBarPanel()
		{
			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("New Project"))
					{
						std::string filepath = HBL2::FileDialogs::SaveFile("Humble Project (*.hblproj)\0*.hblproj\0");

						HBL2::Project::Create()->Save(filepath);

						const auto& startScenePath = HBL2::Project::GetAssetFileSystemPath(HBL2::Project::GetActive()->GetSpecification().StartingScene);

						HBL2::Project::SaveScene(new HBL2::Scene("Empty Scene"), startScenePath);

						HBL2::Project::OpenScene(startScenePath);

						m_Context = HBL2::Context::ActiveScene;
						m_EditorScenePath = filepath;
						m_CurrentDirectory = HBL2::Project::GetAssetDirectory();
					}
					if (ImGui::MenuItem("Open Project"))
					{
						std::string filepath = HBL2::FileDialogs::OpenFile("Humble Project (*.hblproj)\0*.hblproj\0");

						if (HBL2::Project::Load(std::filesystem::path(filepath)) != nullptr)
						{
							const auto& startingScenePath = HBL2::Project::GetAssetFileSystemPath(HBL2::Project::GetActive()->GetSpecification().StartingScene);

							HBL2::Project::OpenScene(startingScenePath);

							m_Context = HBL2::Context::ActiveScene;
							m_EditorScenePath = filepath;
							m_CurrentDirectory = HBL2::Project::GetAssetDirectory();
						}
						else
						{
							HBL2_ERROR("Could not open specified project at path \"{0}\".", filepath);
						}
					}
					if (ImGui::MenuItem("Save Scene"))
					{
						if (m_EditorScenePath.empty())
						{
							std::string filepath = HBL2::FileDialogs::SaveFile("Humble Scene (*.humble)\0*.humble\0");

							HBL2::Project::SaveScene(m_Context, filepath);

							m_EditorScenePath = filepath;
						}
						else
						{
							HBL2::Project::SaveScene(m_Context, m_EditorScenePath);
						}
					}
					if (ImGui::MenuItem("Save Scene As"))
					{
						std::string filepath = HBL2::FileDialogs::SaveFile("Humble Scene (*.humble)\0*.humble\0");

						HBL2::Project::SaveScene(m_Context, filepath);

						m_EditorScenePath = filepath;
					}

					if (ImGui::MenuItem("Open Scene"))
					{
						std::string filepath = HBL2::FileDialogs::OpenFile("Humble Project (*.humble)\0*.humble\0");

						HBL2::Project::OpenScene(filepath);

						m_Context = HBL2::Context::ActiveScene;
						m_EditorScenePath = filepath;
					}
					if (ImGui::MenuItem("Build (Windows)"))
					{
						// TODO: Fix absolute paths.
						// TODO: Get active project to build.

						// Build.
						system("\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\msbuild.exe\" C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\HumbleGameEngine2.sln /t:HumbleApp /p:Configuration=Release");

						// Copy assets to build folder.
						std::filesystem::copy("./EmptyProject", "C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\bin\\Release-x86_64\\HumbleApp\\EmptyProject", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

						// Copy assets to build folder.
						std::filesystem::copy("./assets", "C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\bin\\Release-x86_64\\HumbleApp\\assets", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
					}
					if (ImGui::MenuItem("Build & Run (Windows)"))
					{
						// TODO: Fix absolute paths.
						// TODO: Get active project to build.

						// Build.
						system("\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\msbuild.exe\" C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\HumbleGameEngine2.sln /t:HumbleApp /p:Configuration=Release");

						// Copy assets to build folder.
						std::filesystem::copy("./EmptyProject", "C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\bin\\Release-x86_64\\HumbleApp\\EmptyProject", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

						// Copy assets to build folder.
						std::filesystem::copy("./assets", "C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\bin\\Release-x86_64\\HumbleApp\\assets", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

						// Run.
						system("C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\bin\\Release-x86_64\\HumbleApp\\HumbleApp.exe \"EmptyProject\\EmptyProject.hblproj\"");
					}
					if (ImGui::MenuItem("Build (Web)"))
					{
						// TODO: Fix absolute paths.

						system("C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\Scripts\\emBuildAll.bat");
					}
					if (ImGui::MenuItem("Close"))
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
					HBL2::Context::Core->GetRegistry()
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

		void EditorPanelSystem::DrawConsolePanel(float ts)
		{
			ImGui::Text("Console");
		}

		void EditorPanelSystem::DrawStatsPanel(float ts)
		{
			ImGui::Text("Frame Time: %f", ts);
		}

		void EditorPanelSystem::DrawContentBrowserPanel()
		{
			// Pop up menu when right clicking on an empty space inside the Content Browser panel.
			if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
			{
				if (ImGui::MenuItem("Create Shader"))
				{
					m_OpenShaderSetupPopup = true;
				}

				if (ImGui::MenuItem("Create Material"))
				{
					m_OpenMaterialSetupPopup = true;
				}

				ImGui::EndPopup();
			}

			if (m_OpenShaderSetupPopup)
			{
				ImGui::Begin("Shader Setup", &m_OpenShaderSetupPopup, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

				static char shaderNameBuffer[256] = "New-Shader";
				ImGui::InputText("Shader Name", shaderNameBuffer, 256);

				ImGui::NewLine();

				if (ImGui::Button("OK"))
				{
					auto shaderAssetHandle = AssetManager::Instance->CreateAsset({
						.debugName = "shader-asset",
						.filePath = m_CurrentDirectory / (std::string(shaderNameBuffer) + ".hblshader"),
						.type = AssetType::Shader,
					});

					std::ofstream fout(m_CurrentDirectory / (std::string(shaderNameBuffer) + ".hblshader"), 0);
					fout << "#shader vertex\n\n";
					fout << "#shader fragment\n\n";
					fout.close();

					m_OpenShaderSetupPopup = false;
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel"))
				{
					m_OpenShaderSetupPopup = false;
				}

				ImGui::End();
			}

			if (m_OpenMaterialSetupPopup)
			{
				ImGui::Begin("Material Setup", &m_OpenMaterialSetupPopup, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

				static char materialNameBuffer[256] = "New-Material";
				ImGui::InputText("Material Name", materialNameBuffer, 256);

				ImGui::NewLine();

				static uint32_t shaderAssetHandlePacked = 0;
				ImGui::InputScalar("Shader", ImGuiDataType_U32, (void*)(intptr_t*)&shaderAssetHandlePacked);

				Handle<Asset> shaderAssetHandle = Handle<Asset>::UnPack(shaderAssetHandlePacked);

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Shader"))
					{
						shaderAssetHandlePacked = *((uint32_t*)payload->Data);
						shaderAssetHandle = Handle<Asset>::UnPack(shaderAssetHandlePacked);
						ImGui::EndDragDropTarget();
					}
				}

				Handle<Shader> shaderHandle = AssetManager::Instance->GetAsset<Shader>(shaderAssetHandle);

				ImGui::NewLine();

				static float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
				ImGui::ColorEdit4("AlbedoColor", color);

				static float metalicness = 1.0f;
				ImGui::InputFloat("Metalicness", &metalicness, 0.05f);

				static float roughness = 1.0f;
				ImGui::InputFloat("Roughness", &roughness, 0.05f);

				static uint32_t albedoMapHandlePacked = 0;
				ImGui::InputScalar("AlbedoMap", ImGuiDataType_U32, (void*)(intptr_t*)&albedoMapHandlePacked);

				Handle<Asset> albedoMapAssetHandle = Handle<Asset>::UnPack(albedoMapHandlePacked);

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Texture"))
					{
						albedoMapHandlePacked = *((uint32_t*)payload->Data);
						albedoMapAssetHandle = Handle<Asset>::UnPack(albedoMapHandlePacked);

						if (albedoMapAssetHandle.IsValid())
						{
							std::ofstream fout(AssetManager::Instance->GetAssetMetadata(albedoMapAssetHandle)->FilePath.string() + ".hbltexture", 0);

							YAML::Emitter out;
							out << YAML::BeginMap;
							out << YAML::Key << "Texture" << YAML::Value;
							out << YAML::BeginMap;
							out << YAML::Key << "UUID" << YAML::Value << AssetManager::Instance->GetAssetMetadata(albedoMapAssetHandle)->UUID;
							out << YAML::Key << "Flip" << YAML::Value << false;
							out << YAML::EndMap;
							out << YAML::EndMap;
							fout << out.c_str();
							fout.close();
						}

						ImGui::EndDragDropTarget();
					}
				}

				Handle<Texture> albedoMapHandle = AssetManager::Instance->GetAsset<Texture>(albedoMapAssetHandle);

				ImGui::NewLine();

				if (ImGui::Button("OK"))
				{
					auto materialAssetHandle = AssetManager::Instance->CreateAsset({
						.debugName = "material-asset",
						.filePath = m_CurrentDirectory / (std::string(materialNameBuffer) + ".hblmat"),
						.type = AssetType::Material,
					});

					std::ofstream fout(m_CurrentDirectory / (std::string(materialNameBuffer) + ".hblmat"), 0);

					YAML::Emitter out;
					out << YAML::BeginMap;
					out << YAML::Key << "Material" << YAML::Value;
					out << YAML::BeginMap;
					out << YAML::Key << "UUID" << YAML::Value << AssetManager::Instance->GetAssetMetadata(materialAssetHandle)->UUID;
					out << YAML::Key << "Shader" << YAML::Value << AssetManager::Instance->GetAssetMetadata(shaderAssetHandle)->UUID;
					out << YAML::Key << "AlbedoColor" << YAML::Value << glm::vec4(color[0], color[1], color[2], color[3]);
					out << YAML::Key << "Metalicness" << YAML::Value << metalicness;
					out << YAML::Key << "Roughness" << YAML::Value << roughness;
					if (albedoMapHandle.IsValid())
					{
						out << YAML::Key << "AlbedoMap" << YAML::Value << AssetManager::Instance->GetAssetMetadata(albedoMapAssetHandle)->UUID;
					}
					else
					{
						out << YAML::Key << "AlbedoMap" << YAML::Value << (UUID)0;
					}
					out << YAML::EndMap;
					out << YAML::EndMap;
					fout << out.c_str();
					fout.close();

					m_OpenMaterialSetupPopup = false;
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel"))
				{
					m_OpenMaterialSetupPopup = false;
				}

				ImGui::End();
			}

			float padding = 16.f;
			float thumbnailSize = 128.f;
			float panelWidth = ImGui::GetContentRegionAvail().x;

			int columnCount = (int)(panelWidth / (padding + thumbnailSize));
			columnCount = columnCount < 1 ? 1 : columnCount;

			ImGui::Columns(columnCount, 0, false);

			if (m_CurrentDirectory != HBL2::Project::GetAssetDirectory())
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
				//ImGui::ImageButton((ImTextureID)m_BackIcon, { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });
				ImGui::Button("Back", { thumbnailSize, thumbnailSize });
				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					m_CurrentDirectory = m_CurrentDirectory.parent_path();
				}
				ImGui::TextWrapped("Back");

				ImGui::NextColumn();
			}

			int id = 0;

			for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
			{
				ImGui::PushID(id++);

				const std::string& path = entry.path().string();
				auto relativePath = std::filesystem::relative(entry.path(), HBL2::Project::GetAssetDirectory());
				const std::string extension = entry.path().extension().string();

				if (extension == ".meta")
				{
					ImGui::PopID();
					continue;
				}

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

				/*ImTextureID textureID;

				if (entry.is_directory())
				{
					textureID = (ImTextureID)m_FolderIcon;
				}
				else
				{
					if (extension == ".obj")
					{
						textureID = (ImTextureID)m_ObjIcon;
					}
					else if (extension == ".fbx")
					{
						textureID = (ImTextureID)m_FbxIcon;
					}
					else if (extension == ".mat")
					{
						textureID = (ImTextureID)m_MatIcon;
					}
					else if (extension == ".mtl")
					{
						textureID = (ImTextureID)m_MtlIcon;
					}
					else if (extension == ".shader")
					{
						textureID = (ImTextureID)m_ShaderIcon;
					}
					else if (extension == ".png")
					{
						textureID = (ImTextureID)m_PngIcon;
					}
					else if (extension == ".jpg")
					{
						textureID = (ImTextureID)m_JpgIcon;
					}
					else if (extension == ".mp3")
					{
						textureID = (ImTextureID)m_Mp3Icon;
					}
					else if (extension == ".txt")
					{
						textureID = (ImTextureID)m_TxtIcon;
					}
					else if (extension == ".humble")
					{
						textureID = (ImTextureID)m_SceneIcon;
					}
					else
					{
						textureID = (ImTextureID)m_FileIcon;
					}
				}*/

				// ImGui::ImageButton(textureID, { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });
				ImGui::Button(entry.path().filename().string().c_str(), { thumbnailSize, thumbnailSize });

				UUID assetUUID = std::hash<std::string>()(entry.path().string());
				Handle<Asset> assetHandle;
				Asset* asset = nullptr;

				for (auto handle : AssetManager::Instance->GetRegisteredAssets())
				{
					Asset* currentAsset = AssetManager::Instance->GetAssetMetadata(handle);
					if (currentAsset->UUID == assetUUID)
					{
						assetHandle = handle;
						asset = currentAsset;
						break;
					}
				}

				if (ImGui::BeginDragDropSource())
				{
					uint32_t packedHandle = assetHandle.Pack();

					if (asset != nullptr)
					{
						switch (asset->Type)
						{
						case AssetType::Shader:
							ImGui::SetDragDropPayload("Content_Browser_Item_Shader", (void*)(intptr_t)&packedHandle, sizeof(uint32_t));
							break;
						case AssetType::Texture:
							ImGui::SetDragDropPayload("Content_Browser_Item_Texture", (void*)(intptr_t)&packedHandle, sizeof(uint32_t));
							break;
						case AssetType::Material:
							ImGui::SetDragDropPayload("Content_Browser_Item_Material", (void*)(intptr_t)&packedHandle, sizeof(uint32_t));
							break;
						case AssetType::Mesh:
							ImGui::SetDragDropPayload("Content_Browser_Item_Mesh", (void*)(intptr_t)&packedHandle, sizeof(uint32_t));
							break;
						case AssetType::Scene:
							ImGui::SetDragDropPayload("Content_Browser_Item_Scene", (void*)(intptr_t)&packedHandle, sizeof(uint32_t));
							break;
						default:
							ImGui::SetDragDropPayload("Content_Browser_Item", (void*)(intptr_t)&packedHandle, sizeof(uint32_t));
							break;
						}
					}
					else
					{
						HBL2_CORE_ERROR("Asset at path: {0} and with UUID: {1} has not been registered. Make sure is it registered before use.", entry.path().string(), assetUUID);
						ImGui::SetDragDropPayload("Content_Browser_Item", (void*)(intptr_t)&packedHandle, sizeof(uint32_t));
					}

					ImGui::EndDragDropSource();
				}

				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					if (entry.is_directory())
					{
						m_CurrentDirectory /= entry.path().filename();
					}
				}

				ImGui::TextWrapped(entry.path().filename().string().c_str());

				ImGui::NextColumn();

				ImGui::PopID();
			}

			ImGui::Columns(1);
		}

		void EditorPanelSystem::DrawViewportPanel()
		{
			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

			if (m_ViewportSize != *(glm::vec2*)&viewportPanelSize)
			{
				HBL2::Renderer::Instance->ResizeFrameBuffer((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
				m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

				// TODO: Change this to switch for play and edit mode.
				if (false)
				{
					m_Context->GetRegistry()
						.view<HBL2::Component::Camera>()
						.each([&](HBL2::Component::Camera& camera)
						{
							if (camera.Enabled && camera.Primary)
							{
								camera.AspectRatio = m_ViewportSize.x / m_ViewportSize.y;
							}
						});
				}
				else if (true)
				{
					HBL2::Context::Core->GetRegistry()
						.group<Component::EditorCamera>(entt::get<HBL2::Component::Camera>)
						.each([&](Component::EditorCamera& editorCamera, HBL2::Component::Camera& camera)
						{
							if (editorCamera.Enabled)
							{
								editorCamera.ViewportSize = m_ViewportSize;
								camera.AspectRatio = m_ViewportSize.x / m_ViewportSize.y;
							}
						});
				}
			}

			ImGui::Image(HBL2::Renderer::Instance->GetColorAttachment(), ImVec2{m_ViewportSize.x, m_ViewportSize.y}, ImVec2{0, 1}, ImVec2{1, 0});

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Scene"))
				{
					auto path = HBL2::Project::GetAssetFileSystemPath((const wchar_t*)payload->Data);

					if (path.extension().string() != ".humble")
					{
						HBL2_WARN("Could not load {0} - not a scene file", path.filename().string());
						ImGui::EndDragDropTarget();
						return;
					}

					HBL2::Project::OpenScene(path);

					m_Context = HBL2::Context::ActiveScene;
					m_EditorScenePath = path;

					ImGui::EndDragDropTarget();
				}
			}
		}
	}
}