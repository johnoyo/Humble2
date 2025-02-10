#include "EditorPanelSystem.h"

#include "Humble2\Utilities\FileDialogs.h"
#include "Humble2\Utilities\YamlUtilities.h"
#include "EditorCameraSystem.h"

#include "Resources\Handle.h"
#include "Resources\ResourceManager.h"
#include "Resources\Types.h"
#include "Resources\TypeDescriptors.h"

#include "UI/LayoutLib.h"
#include "Utilities/UnityBuilder.h"

#include<vector>

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
				Scene* currentScene = ResourceManager::Instance->GetScene(sce.CurrentScene);
				if (currentScene != nullptr && currentScene->GetName().find("(Clone)") != std::string::npos)
				{
					// Clear entire scene
					currentScene->Clear();

					// Unload unity build dll.
					NativeScriptUtilities::Get().UnloadUnityBuild(currentScene);

					// Delete play mode scene.
					ResourceManager::Instance->DeleteScene(sce.CurrentScene);
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
			}

			{
				// Properties panel.
				auto propertiesPanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(propertiesPanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(propertiesPanel);
				panel.Name = "Properties";
				panel.Type = Component::EditorPanel::Panel::Properties;
			}

			{
				// Menubar panel.
				auto menubarPanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(menubarPanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(menubarPanel);
				panel.Name = "Menubar";
				panel.Type = Component::EditorPanel::Panel::Menubar;
				panel.UseBeginEnd = false;
			}

			{
				// Stats panel.
				auto statsPanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(statsPanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(statsPanel);
				panel.Name = "Stats";
				panel.Type = Component::EditorPanel::Panel::Stats;
			}

			{
				// Content browser panel.
				auto contentBrowserPanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(contentBrowserPanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(contentBrowserPanel);
				panel.Name = "Content Browser";
				panel.Type = Component::EditorPanel::Panel::ContentBrowser;
				m_CurrentDirectory = HBL2::Project::GetAssetDirectory();
			}

			{
				// Console panel.
				auto consolePanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(consolePanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(consolePanel);
				panel.Name = "Console";
				panel.Type = Component::EditorPanel::Panel::Console;
			}

			{
				// Viewport panel.
				auto viewportPanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(viewportPanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(viewportPanel);
				panel.Name = "Viewport";
				panel.Type = Component::EditorPanel::Panel::Viewport;
				panel.Styles.push_back({ ImGuiStyleVar_WindowPadding, ImVec2{ 0.f, 0.f }, 0.f, false });
			}

			{
				// Play / Stop panel.
				auto playStopPanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(playStopPanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(playStopPanel);
				panel.Name = "Play / Stop";
				panel.Type = Component::EditorPanel::Panel::PlayStop;
				panel.Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
			}

			{
				// Systems panel.
				auto systems = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(systems).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(systems);
				panel.Name = "Systems";
				panel.Type = Component::EditorPanel::Panel::Systems;
			}

			{
				// Bottom tray panel.
				auto bottomTrayPanel = m_Context->CreateEntity();
				m_Context->GetComponent<HBL2::Component::Tag>(bottomTrayPanel).Name = "Hidden";
				auto& panel = m_Context->AddComponent<Component::EditorPanel>(bottomTrayPanel);
				panel.Name = "Bottom Tray";
				panel.Type = Component::EditorPanel::Panel::Tray;
				panel.Flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
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
							if (m_ProjectChanged)
							{
								m_ProjectChanged = false;			// TODO: Fix! Move this inside the DrawToolBarPanel method.
								return;
							}
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
						case Component::EditorPanel::Panel::PlayStop:
							DrawPlayStopPanel();
							break;
						case Component::EditorPanel::Panel::Systems:
							DrawSystemsPanel();
							break;
						case Component::EditorPanel::Panel::Tray:
							DrawTrayPanel();
							break;
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

		void EditorPanelSystem::DrawHierachy(entt::entity entity, const auto& entities)
		{
			const auto& tag = m_ActiveScene->GetComponent<HBL2::Component::Tag>(entity);

			bool selectedEntityCondition = HBL2::Component::EditorVisible::Selected && HBL2::Component::EditorVisible::SelectedEntity == entity;

			ImGuiTreeNodeFlags flags = (selectedEntityCondition ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
			bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.Name.c_str());

			if (ImGui::BeginDragDropSource())
			{
				UUID entityUUID = m_ActiveScene->GetComponent<HBL2::Component::ID>(entity).Identifier;

				if (!m_ActiveScene->HasComponent<HBL2::Component::Link>(entity))
				{
					m_ActiveScene->AddComponent<HBL2::Component::Link>(entity);
				}

				ImGui::SetDragDropPayload("Entity_UUID", (void*)(UUID*)&entityUUID, sizeof(UUID));
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity_UUID"))
				{
					UUID entityUUID = *((UUID*)payload->Data);

					auto childEntity = m_ActiveScene->FindEntityByUUID(entityUUID);

					// Add Link component to child entity if it does not have one.
					if (!m_ActiveScene->HasComponent<HBL2::Component::Link>(childEntity))
					{
						m_ActiveScene->AddComponent<HBL2::Component::Link>(childEntity);
					}

					// Add Link component to this entity(parent of child entity) if it does not have one.
					if (!m_ActiveScene->HasComponent<HBL2::Component::Link>(entity))
					{
						m_ActiveScene->AddComponent<HBL2::Component::Link>(entity);
					}

					// Set the parent of the child entity to be this entity that it was dragged into.
					m_ActiveScene->GetComponent<HBL2::Component::Link>(childEntity).Parent = m_ActiveScene->GetComponent<HBL2::Component::ID>(entity).Identifier;

					ImGui::EndDragDropTarget();
				}
			}

			if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				HBL2::Component::EditorVisible::Selected = true;
				HBL2::Component::EditorVisible::SelectedEntity = entity;
			}

			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Unparent"))
				{
					if (m_ActiveScene->HasComponent<HBL2::Component::Link>(entity))
					{
						m_ActiveScene->GetComponent<HBL2::Component::Link>(entity).Parent = 0;
					}
				}
				else if (ImGui::MenuItem("Destroy"))
				{
					// Defer the deletion at the end of the function, for now just mark the entity.
					m_EntityToBeDeleted = entity;
				}
				else if (ImGui::MenuItem("Duplicate"))
				{
					m_ActiveScene->DuplicateEntity(entity);
				}

				ImGui::EndPopup();
			}

			if (opened)
			{
				for (const auto e : entities)
				{
					if (m_ActiveScene->HasComponent<HBL2::Component::Link>(e))
					{
						const auto parentEntityUUID = m_ActiveScene->GetComponent<HBL2::Component::Link>(e).Parent;
						auto parentEntity = m_ActiveScene->FindEntityByUUID(parentEntityUUID);

						if (parentEntity == entity)
						{
							DrawHierachy(e, entities);
						}
					}
				}

				ImGui::TreePop();
			}
		}

		void EditorPanelSystem::DrawHierachyPanel()
		{
			if (m_ActiveScene == nullptr)
			{
				return;
			}

			// Pop up menu when right clicking on an empty space inside the hierachy panel.
			if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
			{
				if (ImGui::MenuItem("Create Empty"))
				{
					auto entity = HBL2::EntityPreset::CreateEmpty();

					HBL2::Component::EditorVisible::SelectedEntity = entity;
					HBL2::Component::EditorVisible::Selected = true;
				}

				if (ImGui::MenuItem("Create Camera"))
				{
					auto entity = HBL2::EntityPreset::CreateCamera();

					HBL2::Component::EditorVisible::SelectedEntity = entity;
					HBL2::Component::EditorVisible::Selected = true;
				}

				if (ImGui::MenuItem("Create Sprite_New"))
				{
					auto entity = HBL2::EntityPreset::CreateSprite();

					HBL2::Component::EditorVisible::SelectedEntity = entity;
					HBL2::Component::EditorVisible::Selected = true;
				}

				if (ImGui::MenuItem("Create Plane"))
				{
					auto entity = HBL2::EntityPreset::CreatePlane();

					HBL2::Component::EditorVisible::SelectedEntity = entity;
					HBL2::Component::EditorVisible::Selected = true;
				}

				if (ImGui::MenuItem("Create Sphere"))
				{
					auto entity = HBL2::EntityPreset::CreateSphere();

					HBL2::Component::EditorVisible::SelectedEntity = entity;
					HBL2::Component::EditorVisible::Selected = true;
				}

				if (ImGui::MenuItem("Create Light"))
				{
					auto entity = HBL2::EntityPreset::CreateLight();

					HBL2::Component::EditorVisible::SelectedEntity = entity;
					HBL2::Component::EditorVisible::Selected = true;
				}

				ImGui::EndPopup();
			}

			m_EntityToBeDeleted = entt::null;

			const auto& entities = m_ActiveScene->GetRegistry().view<entt::entity>();

			if (ImGui::CollapsingHeader("Entity Hierarchy", ImGuiTreeNodeFlags_DefaultOpen))
			{
				for (const auto entity : entities)
				{
					// Only display entities that have no parent (i.e., true root entities)
					// The child nodes will be displayed recursively in the DrawHierachy method.
					if (m_ActiveScene->HasComponent<HBL2::Component::Link>(entity))
					{
						const auto parentEntityUUID = m_ActiveScene->GetComponent<HBL2::Component::Link>(entity).Parent;
						if (parentEntityUUID == 0)
						{
							DrawHierachy(entity, entities);
						}
					}
					else
					{
						DrawHierachy(entity, entities);
					}
				}
			}

			if (m_EntityToBeDeleted != entt::null)
			{
				// Destroy entity and clear entityToBeDeleted value.
				m_ActiveScene->DestroyEntity(m_EntityToBeDeleted);
				m_EntityToBeDeleted = entt::null;

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
				if (m_ActiveScene->HasComponent<HBL2::Component::Tag>(HBL2::Component::EditorVisible::SelectedEntity))
				{
					auto& tag = m_ActiveScene->GetComponent<HBL2::Component::Tag>(HBL2::Component::EditorVisible::SelectedEntity).Name;

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
				if (m_ActiveScene->HasComponent<HBL2::Component::Transform>(HBL2::Component::EditorVisible::SelectedEntity))
				{
					if (ImGui::TreeNodeEx((void*)typeid(HBL2::Component::Transform).hash_code(), treeNodeFlags, "Transform"))
					{
						auto& transform = m_ActiveScene->GetComponent<HBL2::Component::Transform>(HBL2::Component::EditorVisible::SelectedEntity);

						// HBL2::EditorUtilities::Get().DrawDefaultEditor<HBL2::Component::Transform>(transform);

						ImGui::DragFloat3("Translation", glm::value_ptr(transform.Translation), 0.25f);
						ImGui::DragFloat3("Rotation", glm::value_ptr(transform.Rotation), 0.25f);
						ImGui::DragFloat3("Scale", glm::value_ptr(transform.Scale), 0.25f);

						ImGui::TreePop();
					}

					ImGui::Separator();
				}

				// Link component.
				if (m_ActiveScene->HasComponent<HBL2::Component::Link>(HBL2::Component::EditorVisible::SelectedEntity))
				{
					bool opened = ImGui::TreeNodeEx((void*)typeid(HBL2::Component::Link).hash_code(), treeNodeFlags, "Link");

					ImGui::SameLine(ImGui::GetWindowWidth() - 25.f);

					bool removeComponent = false;

					if (ImGui::Button("-", ImVec2{ 18.f, 18.f }))
					{
						removeComponent = true;
					}

					if (opened)
					{
						auto& link = m_ActiveScene->GetComponent<HBL2::Component::Link>(HBL2::Component::EditorVisible::SelectedEntity);

						bool renderBaseEditor = true;

						if (HBL2::EditorUtilities::Get().HasCustomEditor<HBL2::Component::Link>())
						{
							renderBaseEditor = HBL2::EditorUtilities::Get().DrawCustomEditor<HBL2::Component::Link, LinkEditor>(link);
						}

						if (renderBaseEditor)
						{
							ImGui::InputScalar("Parent", ImGuiDataType_U32, &link.Parent);

							if (ImGui::BeginDragDropTarget())
							{
								if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity_UUID"))
								{
									UUID parentEntityUUID = *((UUID*)payload->Data);
									UUID childEntityUUID = m_ActiveScene->GetComponent<HBL2::Component::ID>(HBL2::Component::EditorVisible::SelectedEntity).Identifier;
									if (childEntityUUID != parentEntityUUID)
									{
										link.Parent = parentEntityUUID;
									}
									ImGui::EndDragDropTarget();
								}
							}

							if (ImGui::BeginPopupContextItem())
							{
								if (ImGui::MenuItem("Unparent"))
								{
									link.Parent = 0;
								}

								ImGui::EndPopup();
							}
						}

						ImGui::TreePop();
					}

					ImGui::Separator();

					if (removeComponent)
					{
						m_ActiveScene->RemoveComponent<HBL2::Component::Link>(HBL2::Component::EditorVisible::SelectedEntity);
					}
				}

				// Camera component.
				if (m_ActiveScene->HasComponent<HBL2::Component::Camera>(HBL2::Component::EditorVisible::SelectedEntity))
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
						auto& camera = m_ActiveScene->GetComponent<HBL2::Component::Camera>(HBL2::Component::EditorVisible::SelectedEntity);

						bool renderBaseEditor = true;

						if (HBL2::EditorUtilities::Get().HasCustomEditor<HBL2::Component::Camera>())
						{
							renderBaseEditor = HBL2::EditorUtilities::Get().DrawCustomEditor<HBL2::Component::Camera, CameraEditor>(camera);
						}

						if (renderBaseEditor)
						{
							ImGui::Checkbox("Enabled", &camera.Enabled);
							ImGui::Checkbox("Primary", &camera.Primary);
							ImGui::SliderFloat("Far", &camera.Far, 0, 100);
							ImGui::SliderFloat("Near", &camera.Near, 100, 1500);
							ImGui::SliderFloat("FOV", &camera.Fov, 0, 120);
							ImGui::SliderFloat("Aspect Ratio", &camera.AspectRatio, 0, 2);
							ImGui::SliderFloat("Zoom Level", &camera.ZoomLevel, 0, 500);

							std::string selectedProjection = camera.Type == HBL2::Component::Camera::Type::Perspective ? "Perspective" : "Orthographic";
							std::string projectionTypes[2] = { "Perspective", "Orthographic" };

							if (ImGui::BeginCombo("Type", selectedProjection.c_str()))
							{
								for (const auto& type : projectionTypes)
								{
									bool isSelected = (selectedProjection == type);
									if (ImGui::Selectable(type.c_str(), isSelected))
									{
										selectedProjection = type;
									}

									if (isSelected)
									{
										ImGui::SetItemDefaultFocus();
									}
								}
								ImGui::EndCombo();
							}

							camera.Type = selectedProjection == "Perspective" ? HBL2::Component::Camera::Type::Perspective : HBL2::Component::Camera::Type::Orthographic;
						}

						ImGui::TreePop();
					}

					ImGui::Separator();

					if (removeComponent)
					{
						m_ActiveScene->RemoveComponent<HBL2::Component::Camera>(HBL2::Component::EditorVisible::SelectedEntity);
					}
				}

				// Sprite_New component.
				if (m_ActiveScene->HasComponent<HBL2::Component::Sprite_New>(HBL2::Component::EditorVisible::SelectedEntity))
				{
					bool opened = ImGui::TreeNodeEx((void*)typeid(HBL2::Component::Sprite_New).hash_code(), treeNodeFlags, "Sprite New");

					ImGui::SameLine(ImGui::GetWindowWidth() - 25.f);

					bool removeComponent = false;

					if (ImGui::Button("-", ImVec2{ 18.f, 18.f }))
					{
						removeComponent = true;
					}

					if (opened)
					{
						auto& sprite = m_ActiveScene->GetComponent<HBL2::Component::Sprite_New>(HBL2::Component::EditorVisible::SelectedEntity);

						uint32_t materialHandle = sprite.Material.Pack();

						ImGui::Checkbox("Enabled", &sprite.Enabled);

						ImGui::InputScalar("Material", ImGuiDataType_U32, (void*)(intptr_t*)&materialHandle);

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Material"))
							{
								uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
								Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

								sprite.Material = AssetManager::Instance->GetAsset<Material>(assetHandle);

								ImGui::EndDragDropTarget();
							}
						}

						ImGui::TreePop();
					}

					ImGui::Separator();

					if (removeComponent)
					{
						m_ActiveScene->RemoveComponent<HBL2::Component::StaticMesh_New>(HBL2::Component::EditorVisible::SelectedEntity);
					}
				}

				// StaticMesh_New component.
				if (m_ActiveScene->HasComponent<HBL2::Component::StaticMesh_New>(HBL2::Component::EditorVisible::SelectedEntity))
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
						auto& mesh = m_ActiveScene->GetComponent<HBL2::Component::StaticMesh_New>(HBL2::Component::EditorVisible::SelectedEntity);
						
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

								if (assetHandle.IsValid())
								{
									std::ofstream fout(HBL2::Project::GetAssetFileSystemPath(AssetManager::Instance->GetAssetMetadata(assetHandle)->FilePath).string() + ".hblmesh", 0);

									YAML::Emitter out;
									out << YAML::BeginMap;
									out << YAML::Key << "Mesh" << YAML::Value;
									out << YAML::BeginMap;
									out << YAML::Key << "UUID" << YAML::Value << AssetManager::Instance->GetAssetMetadata(assetHandle)->UUID;
									out << YAML::EndMap;
									out << YAML::EndMap;
									fout << out.c_str();
									fout.close();
								}

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
						m_ActiveScene->RemoveComponent<HBL2::Component::StaticMesh_New>(HBL2::Component::EditorVisible::SelectedEntity);
					}
				}

				// Light component.
				if (m_ActiveScene->HasComponent<HBL2::Component::Light>(HBL2::Component::EditorVisible::SelectedEntity))
				{
					bool opened = ImGui::TreeNodeEx((void*)typeid(HBL2::Component::Light).hash_code(), treeNodeFlags, "Light");

					ImGui::SameLine(ImGui::GetWindowWidth() - 25.f);

					bool removeComponent = false;

					if (ImGui::Button("-", ImVec2{ 18.f, 18.f }))
					{
						removeComponent = true;
					}

					if (opened)
					{
						auto& light = m_ActiveScene->GetComponent<HBL2::Component::Light>(HBL2::Component::EditorVisible::SelectedEntity);

						ImGui::Checkbox("Enabled", &light.Enabled);
						std::string selectedType = light.Type == HBL2::Component::Light::Type::Directional ? "Directional" : "Point";
						std::string lightTypes[2] = { "Directional", "Point" };

						if (ImGui::BeginCombo("Type", selectedType.c_str()))
						{
							for (const auto& type : lightTypes)
							{
								bool isSelected = (selectedType == type);
								if (ImGui::Selectable(type.c_str(), isSelected))
								{
									selectedType = type;
								}

								if (isSelected)
								{
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}
						ImGui::Checkbox("CastsShadows", &light.CastsShadows);
						ImGui::SliderFloat("Intensity", &light.Intensity, 0, 20);
						if (light.Type == HBL2::Component::Light::Type::Point)
						{
							ImGui::SliderFloat("Attenuation", &light.Attenuation, 0, 100);
						}
						ImGui::ColorEdit3("Color", glm::value_ptr(light.Color));

						light.Type = selectedType == "Directional" ? HBL2::Component::Light::Type::Directional : HBL2::Component::Light::Type::Point;

						ImGui::TreePop();
					}

					ImGui::Separator();

					if (removeComponent)
					{
						m_ActiveScene->RemoveComponent<HBL2::Component::Light>(HBL2::Component::EditorVisible::SelectedEntity);
					}
				}

				using namespace entt::literals;

				// Iterate over all registered meta types
				for (auto meta_type : entt::resolve(m_ActiveScene->GetMetaContext()))
				{
					std::string componentName = meta_type.second.info().name().data();
					componentName = NativeScriptUtilities::Get().CleanComponentNameO3(componentName);

					if (NativeScriptUtilities::Get().HasComponent(componentName, m_ActiveScene, HBL2::Component::EditorVisible::SelectedEntity))
					{
						bool opened = ImGui::TreeNodeEx((void*)meta_type.second.info().hash(), treeNodeFlags, componentName.c_str());

						ImGui::SameLine(ImGui::GetWindowWidth() - 25.f);

						bool removeComponent = false;

						if (ImGui::Button("-", ImVec2{ 18.f, 18.f }))
						{
							removeComponent = true;
						}

						if (opened)
						{
							entt::meta_any componentMeta = HBL2::NativeScriptUtilities::Get().GetComponent(componentName, m_ActiveScene, HBL2::Component::EditorVisible::SelectedEntity);

							HBL2::EditorUtilities::Get().DrawDefaultEditor(componentMeta);

							ImGui::TreePop();
						}

						ImGui::Separator();

						if (removeComponent)
						{
							HBL2::NativeScriptUtilities::Get().RemoveComponent(componentName, m_ActiveScene, HBL2::Component::EditorVisible::SelectedEntity);
						}
					}
				}

				// Add component button.
				if (ImGui::Button("Add Component"))
				{
					ImGui::OpenPopup("AddComponent");
				}

				if (ImGui::BeginPopup("AddComponent"))
				{
					if (ImGui::MenuItem("Sprite_New"))
					{
						m_ActiveScene->AddComponent<HBL2::Component::Sprite_New>(HBL2::Component::EditorVisible::SelectedEntity);
						ImGui::CloseCurrentPopup();
					}

					if (ImGui::MenuItem("StaticMesh_New"))
					{
						m_ActiveScene->AddComponent<HBL2::Component::StaticMesh_New>(HBL2::Component::EditorVisible::SelectedEntity);
						ImGui::CloseCurrentPopup();
					}

					if (ImGui::MenuItem("Camera"))
					{
						m_ActiveScene->AddComponent<HBL2::Component::Camera>(HBL2::Component::EditorVisible::SelectedEntity);
						ImGui::CloseCurrentPopup();
					}

					if (ImGui::MenuItem("Link"))
					{
						m_ActiveScene->AddComponent<HBL2::Component::Link>(HBL2::Component::EditorVisible::SelectedEntity);
						ImGui::CloseCurrentPopup();
					}

					if (ImGui::MenuItem("Light"))
					{
						m_ActiveScene->AddComponent<HBL2::Component::Light>(HBL2::Component::EditorVisible::SelectedEntity);
						ImGui::CloseCurrentPopup();
					}

					// Iterate over all registered meta types
					for (auto meta_type : entt::resolve(m_ActiveScene->GetMetaContext()))
					{
						std::string componentName = meta_type.second.info().name().data();
						componentName = NativeScriptUtilities::Get().CleanComponentNameO3(componentName);

						if (ImGui::MenuItem(componentName.c_str()))
						{
							HBL2::NativeScriptUtilities::Get().AddComponent(componentName, m_ActiveScene, HBL2::Component::EditorVisible::SelectedEntity);
							ImGui::CloseCurrentPopup();
						}
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

						std::string projectName = std::filesystem::path(filepath).filename().stem().string();

						AssetManager::Instance->DeregisterAssets();

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
					if (ImGui::MenuItem("Open Project"))
					{
						std::string filepath = HBL2::FileDialogs::OpenFile("Humble Project (*.hblproj)\0*.hblproj\0");

						AssetManager::Instance->DeregisterAssets();

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
					if (ImGui::MenuItem("Save Scene"))
					{
						if (m_EditorScenePath.empty())
						{
							std::string filepath = HBL2::FileDialogs::SaveFile("Humble Scene (*.humble)\0*.humble\0");
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
					if (ImGui::MenuItem("Save Scene As"))
					{
						std::string filepath = HBL2::FileDialogs::SaveFile("Humble Scene (*.humble)\0*.humble\0");
						auto relativePath = std::filesystem::relative(std::filesystem::path(filepath), HBL2::Project::GetAssetDirectory());
						const char* sceneName = relativePath.filename().stem().string().c_str();

						auto assetHandle = AssetManager::Instance->CreateAsset({
							.debugName = sceneName,
							.filePath = relativePath,
							.type = AssetType::Scene,
						});

						AssetManager::Instance->SaveAsset(assetHandle);
						Asset* asset = AssetManager::Instance->GetAssetMetadata(assetHandle);

						Handle<Scene> sceneHandle = Handle<Scene>::UnPack(asset->Indentifier);

						Scene* playScene = ResourceManager::Instance->GetScene(sceneHandle);
						Scene::Copy(m_ActiveScene, playScene);
						AssetManager::Instance->SaveAsset(assetHandle);

						SceneManager::Get().LoadScene(assetHandle);

						m_EditorScenePath = filepath;
					}
					if (ImGui::MenuItem("New Scene"))
					{
						std::string filepath = HBL2::FileDialogs::SaveFile("Humble Project (*.humble)\0*.humble\0");
						auto relativePath = std::filesystem::relative(std::filesystem::path(filepath), HBL2::Project::GetAssetDirectory());

						auto assetHandle = AssetManager::Instance->CreateAsset({
							.debugName = "New Scene",
							.filePath = relativePath,
							.type = AssetType::Scene,
						});

						AssetManager::Instance->SaveAsset(std::hash<std::string>()(relativePath.string()));

						HBL2::SceneManager::Get().LoadScene(assetHandle);

						m_EditorScenePath = filepath;
					}
					if (ImGui::MenuItem("Open Scene"))
					{
						std::string filepath = HBL2::FileDialogs::OpenFile("Humble Project (*.humble)\0*.humble\0");
						auto relativePath = std::filesystem::relative(std::filesystem::path(filepath), HBL2::Project::GetAssetDirectory());
						UUID sceneUUID = std::hash<std::string>()(relativePath.string());

						HBL2::SceneManager::Get().LoadScene(AssetManager::Instance->GetHandleFromUUID(sceneUUID));

						m_EditorScenePath = filepath;
					}
					if (ImGui::MenuItem("Build (Windows - Debug)"))
					{
						const std::string& projectName = HBL2::Project::GetActive()->GetSpecification().Name;

						// Build.
						system("\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\msbuild.exe\" ..\\HumbleGameEngine2.sln /t:HumbleApp /p:Configuration=Debug");

						// Copy assets to build folder.
						std::filesystem::copy("./" + projectName, "..\\bin\\Debug-x86_64\\HumbleApp\\" + projectName, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

						// Copy assets to build folder.
						std::filesystem::copy("./assets", "..\\bin\\Debug-x86_64\\HumbleApp\\assets", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
					}
					if (ImGui::MenuItem("Build (Windows - Release)"))
					{
						const std::string& projectName = HBL2::Project::GetActive()->GetSpecification().Name;

						// Build.
						system("\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\msbuild.exe\" ..\\HumbleGameEngine2.sln /t:HumbleApp /p:Configuration=Release");

						// Copy assets to build folder.
						std::filesystem::copy("./" + projectName, "..\\bin\\Release-x86_64\\HumbleApp\\" + projectName, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

						// Copy assets to build folder.
						std::filesystem::copy("./assets", "..\\bin\\Release-x86_64\\HumbleApp\\assets", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
					}
					if (ImGui::MenuItem("Build & Run (Windows - Debug)"))
					{
						const std::string& projectName = HBL2::Project::GetActive()->GetSpecification().Name;

						// Build.
						system("\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\msbuild.exe\" ..\\HumbleGameEngine2.sln /t:HumbleApp /p:Configuration=Debug");

						// Copy assets to build folder.
						std::filesystem::copy("./" + projectName, "..\\bin\\Debug-x86_64\\HumbleApp\\" + projectName, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

						// Copy assets to build folder.
						std::filesystem::copy("./assets", "..\\bin\\Debug-x86_64\\HumbleApp\\assets", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

						// Run.
						system("cd ..\\bin\\Debug-x86_64\\HumbleApp && HumbleApp.exe");
					}
					if (ImGui::MenuItem("Build & Run (Windows - Release)"))
					{
						const std::string& projectName = HBL2::Project::GetActive()->GetSpecification().Name;

						// Build.
						system("\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\msbuild.exe\" ..\\HumbleGameEngine2.sln /t:HumbleApp /p:Configuration=Release");

						// Copy assets to build folder.
						std::filesystem::copy("./" + projectName, "..\\bin\\Release-x86_64\\HumbleApp\\" + projectName, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

						// Copy assets to build folder.
						std::filesystem::copy("./assets", "..\\bin\\Release-x86_64\\HumbleApp\\assets", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

						// Run.
						system("cd ..\\bin\\Release-x86_64\\HumbleApp && HumbleApp.exe");
					}
					if (ImGui::MenuItem("Build (Web)"))
					{
						system("..\\Scripts\\emBuildAll.bat");
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
				if (ImGui::BeginMenu("Create"))
				{
					if (ImGui::MenuItem("Folder"))
					{
						m_OpenNewFolderSetupPopup = true;
					}

					if (ImGui::BeginMenu("Shader"))
					{
						if (ImGui::MenuItem("Unlit"))
						{
							m_SelectedShaderType = 0;
							m_OpenShaderSetupPopup = true;
						}

						if (ImGui::MenuItem("Blinn-Phong"))
						{
							m_SelectedShaderType = 1;
							m_OpenShaderSetupPopup = true;
						}

						if (ImGui::MenuItem("PBR"))
						{
							m_SelectedShaderType = 2;
							m_OpenShaderSetupPopup = true;
						}

						ImGui::EndMenu();
					}

					if (ImGui::BeginMenu("Material"))
					{
						if (ImGui::MenuItem("Unlit"))
						{
							m_SelectedMaterialType = 0;
							m_OpenMaterialSetupPopup = true;
						}

						if (ImGui::MenuItem("Blinn-Phong"))
						{
							m_SelectedMaterialType = 1;
							m_OpenMaterialSetupPopup = true;
						}

						if (ImGui::MenuItem("PBR"))
						{
							m_SelectedMaterialType = 2;
							m_OpenMaterialSetupPopup = true;
						}

						ImGui::EndMenu();
					}

					if (ImGui::MenuItem("System"))
					{
						m_OpenScriptSetupPopup = true;
					}

					if (ImGui::MenuItem("Component"))
					{
						m_OpenComponentSetupPopup = true;
					}

					if (ImGui::MenuItem("Helper Script"))
					{
						m_OpenHelperScriptSetupPopup = true;
					}

					ImGui::EndMenu();
				}

				ImGui::EndPopup();
			}

			if (m_OpenNewFolderSetupPopup)
			{
				ImGui::Begin("New Folder", &m_OpenNewFolderSetupPopup, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

				static char folderNameBuffer[256] = "NewFolder";
				ImGui::InputText("Folder Name", folderNameBuffer, 256);

				ImGui::NewLine();

				if (ImGui::Button("OK"))
				{
					try
					{
						std::filesystem::create_directory(m_CurrentDirectory / folderNameBuffer);
					}
					catch (std::exception& e)
					{
						HBL2_ERROR("New folder creation failed: {0}", e.what());
					}

					m_OpenNewFolderSetupPopup = false;
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel"))
				{
					m_OpenNewFolderSetupPopup = false;
				}

				ImGui::End();
			}

			if (m_OpenScriptSetupPopup)
			{
				ImGui::Begin("System Setup", &m_OpenScriptSetupPopup, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

				static char systemNameBuffer[256] = "NewSystem";
				ImGui::InputText("System Name", systemNameBuffer, 256);

				ImGui::NewLine();

				if (ImGui::Button("OK"))
				{
					// Create .h file with placeholder code.
					auto scriptAssetHandle = NativeScriptUtilities::Get().CreateSystemFile(m_CurrentDirectory, systemNameBuffer);

					// Import script.
					AssetManager::Instance->GetAsset<Script>(scriptAssetHandle);

					if (Context::Mode == Mode::Runtime)
					{
						HBL2_WARN("Hot reloading is not available yet. Skipping requested recompilation. Recompile script after leaving Play mode.");
					}
					else
					{
						// Save script (build).
						AssetManager::Instance->SaveAsset(scriptAssetHandle);
					}

					m_OpenScriptSetupPopup = false;
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel"))
				{
					m_OpenScriptSetupPopup = false;
				}

				ImGui::End();
			}

			if (m_OpenComponentSetupPopup)
			{
				ImGui::Begin("Component Setup", &m_OpenComponentSetupPopup, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

				static char componentNameBuffer[256] = "NewComponent";
				ImGui::InputText("Component Name", componentNameBuffer, 256);

				ImGui::NewLine();

				if (ImGui::Button("OK"))
				{
					// Create .h file with placeholder code.
					auto scriptAssetHandle = NativeScriptUtilities::Get().CreateComponentFile(m_CurrentDirectory, componentNameBuffer);

					// Import script.
					AssetManager::Instance->GetAsset<Script>(scriptAssetHandle);

					if (Context::Mode == Mode::Runtime)
					{
						HBL2_WARN("Hot reloading is not available yet. Skipping requested recompilation. Recompile script after leaving Play mode.");
					}
					else
					{
						// Save script (build).
						AssetManager::Instance->SaveAsset(scriptAssetHandle);
					}

					m_OpenComponentSetupPopup = false;
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel"))
				{
					m_OpenComponentSetupPopup = false;
				}

				ImGui::End();
			}

			if (m_OpenHelperScriptSetupPopup)
			{
				ImGui::Begin("Helper Script Setup", &m_OpenHelperScriptSetupPopup, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

				static char scriptNameBuffer[256] = "NewHelperScript";
				ImGui::InputText("Script Name", scriptNameBuffer, 256);

				ImGui::NewLine();

				if (ImGui::Button("OK"))
				{
					// Create .h file with placeholder code.
					auto scriptAssetHandle = NativeScriptUtilities::Get().CreateHelperScriptFile(m_CurrentDirectory, scriptNameBuffer);

					// Import script.
					AssetManager::Instance->GetAsset<Script>(scriptAssetHandle);

					if (Context::Mode == Mode::Runtime)
					{
						HBL2_WARN("Hot reloading is not available yet. Skipping requested recompilation. Recompile script after leaving Play mode.");
					}
					else
					{
						// Save script (build).
						AssetManager::Instance->SaveAsset(scriptAssetHandle);
					}

					m_OpenHelperScriptSetupPopup = false;
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel"))
				{
					m_OpenHelperScriptSetupPopup = false;
				}

				ImGui::End();
			}

			if (m_OpenShaderSetupPopup)
			{
				ImGui::Begin("Shader Setup", &m_OpenShaderSetupPopup, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

				static char shaderNameBuffer[256] = "New-Shader";
				ImGui::InputText("Shader Name", shaderNameBuffer, 256);

				ImGui::NewLine();

				if (ImGui::Button("OK"))
				{
					auto relativePath = std::filesystem::relative(m_CurrentDirectory / (std::string(shaderNameBuffer) + ".shader"), HBL2::Project::GetAssetDirectory());

					auto shaderAssetHandle = AssetManager::Instance->CreateAsset({
						.debugName = "shader-asset",
						.filePath = relativePath,
						.type = AssetType::Shader,
					});

					std::string shaderSource;

					switch (m_SelectedShaderType)
					{
					case 0:
						shaderSource = ShaderUtilities::Get().ReadFile("assets/shaders/unlit.shader");
						break;
					case 1:
						shaderSource = ShaderUtilities::Get().ReadFile("assets/shaders/blinn-phong.shader");
						break;
					case 2:
						shaderSource = ShaderUtilities::Get().ReadFile("assets/shaders/pbr.shader");
						break;
					}

					if (shaderAssetHandle.IsValid())
					{
						ShaderUtilities::Get().CreateShaderMetadataFile(shaderAssetHandle, m_SelectedShaderType);
					}

					std::ofstream fout(m_CurrentDirectory / (std::string(shaderNameBuffer) + ".shader"), 0);
					fout << shaderSource;
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
							TextureUtilities::Get().CreateAssetMetadataFile(albedoMapAssetHandle);
						}

						ImGui::EndDragDropTarget();
					}
				}

				Handle<Texture> albedoMapHandle = AssetManager::Instance->GetAsset<Texture>(albedoMapAssetHandle);

				Handle<Asset> normalMapAssetHandle;
				Handle<Asset> metallicMapAssetHandle;
				Handle<Asset> roughnessMapAssetHandle;

				if (m_SelectedMaterialType == 2)
				{
					// Normal map
					static uint32_t normalMapHandlePacked = 0;
					ImGui::InputScalar("NormalMap", ImGuiDataType_U32, (void*)(intptr_t*)&normalMapHandlePacked);

					normalMapAssetHandle = Handle<Asset>::UnPack(normalMapHandlePacked);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Texture"))
						{
							normalMapHandlePacked = *((uint32_t*)payload->Data);
							normalMapAssetHandle = Handle<Asset>::UnPack(normalMapHandlePacked);

							if (normalMapAssetHandle.IsValid())
							{
								TextureUtilities::Get().CreateAssetMetadataFile(normalMapAssetHandle);
							}

							ImGui::EndDragDropTarget();
						}
					}


					// Metalicness map
					static uint32_t metallicMapHandlePacked = 0;
					ImGui::InputScalar("MetallicMap", ImGuiDataType_U32, (void*)(intptr_t*)&metallicMapHandlePacked);

					metallicMapAssetHandle = Handle<Asset>::UnPack(metallicMapHandlePacked);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Texture"))
						{
							metallicMapHandlePacked = *((uint32_t*)payload->Data);
							metallicMapAssetHandle = Handle<Asset>::UnPack(metallicMapHandlePacked);

							if (metallicMapAssetHandle.IsValid())
							{
								TextureUtilities::Get().CreateAssetMetadataFile(metallicMapAssetHandle);
							}

							ImGui::EndDragDropTarget();
						}
					}


					// Roughness map
					static uint32_t roughnessMapHandlePacked = 0;
					ImGui::InputScalar("RoughnessMap", ImGuiDataType_U32, (void*)(intptr_t*)&roughnessMapHandlePacked);

					roughnessMapAssetHandle = Handle<Asset>::UnPack(roughnessMapHandlePacked);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Texture"))
						{
							roughnessMapHandlePacked = *((uint32_t*)payload->Data);
							roughnessMapAssetHandle = Handle<Asset>::UnPack(roughnessMapHandlePacked);

							if (roughnessMapAssetHandle.IsValid())
							{
								TextureUtilities::Get().CreateAssetMetadataFile(roughnessMapAssetHandle);
							}

							ImGui::EndDragDropTarget();
						}
					}
				}

				Handle<Texture> normalMapHandle = AssetManager::Instance->GetAsset<Texture>(normalMapAssetHandle);
				Handle<Texture> metallicMapHandle = AssetManager::Instance->GetAsset<Texture>(metallicMapAssetHandle);
				Handle<Texture> roughnessMapHandle = AssetManager::Instance->GetAsset<Texture>(roughnessMapAssetHandle);

				ImGui::NewLine();

				if (ImGui::Button("OK"))
				{
					auto relativePath = std::filesystem::relative(m_CurrentDirectory / (std::string(materialNameBuffer) + ".mat"), HBL2::Project::GetAssetDirectory());

					auto materialAssetHandle = AssetManager::Instance->CreateAsset({
						.debugName = "material-asset",
						.filePath = relativePath,
						.type = AssetType::Material,
					});

					if (materialAssetHandle.IsValid())
					{
						ShaderUtilities::Get().CreateMaterialMetadataFile(materialAssetHandle, m_SelectedMaterialType);
					}

					std::ofstream fout(m_CurrentDirectory / (std::string(materialNameBuffer) + ".mat"), 0);

					YAML::Emitter out;
					out << YAML::BeginMap;
					out << YAML::Key << "Material" << YAML::Value;
					out << YAML::BeginMap;
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
					if (normalMapHandle.IsValid())
					{
						out << YAML::Key << "NormalMap" << YAML::Value << AssetManager::Instance->GetAssetMetadata(normalMapAssetHandle)->UUID;
					}
					else
					{
						out << YAML::Key << "NormalMap" << YAML::Value << (UUID)0;
					}
					if (metallicMapHandle.IsValid())
					{
						out << YAML::Key << "MetallicMap" << YAML::Value << AssetManager::Instance->GetAssetMetadata(metallicMapAssetHandle)->UUID;
					}
					else
					{
						out << YAML::Key << "MetallicMap" << YAML::Value << (UUID)0;
					}
					if (roughnessMapHandle.IsValid())
					{
						out << YAML::Key << "RoughnessMap" << YAML::Value << AssetManager::Instance->GetAssetMetadata(roughnessMapAssetHandle)->UUID;
					}
					else
					{
						out << YAML::Key << "RoughnessMap" << YAML::Value << (UUID)0;
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

				// Do not show engine metadata files.
				if (extension.find(".hbl") != std::string::npos)
				{
					ImGui::PopID();
					continue;
				}

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

				ImGui::Button(entry.path().filename().string().c_str(), { thumbnailSize, thumbnailSize });

				UUID assetUUID = std::hash<std::string>()(relativePath.string());
				Handle<Asset> assetHandle = AssetManager::Instance->GetHandleFromUUID(assetUUID);
				Asset* asset = AssetManager::Instance->GetAssetMetadata(assetHandle);

				if (ImGui::BeginPopupContextItem())
				{
					if (extension == ".h")
					{

						if (ImGui::MenuItem("Recompile"))
						{
							if (Context::Mode == Mode::Runtime)
							{
								HBL2_WARN("Hot reloading is not available yet. Skipping requested recompilation.");
							}
							else
							{
								AssetManager::Instance->GetAsset<Script>(assetHandle);
								AssetManager::Instance->SaveAsset(assetHandle);				// NOTE: Consider changing this!
							}
						}
					}

					if (ImGui::MenuItem("Delete"))
					{
						m_OpenDeleteConfirmationWindow = true;
						m_AssetToBeDeleted = assetHandle;
					}					

					ImGui::EndPopup();
				}

				if (ImGui::BeginDragDropSource())
				{
					uint32_t packedHandle = assetHandle.Pack();

					if (asset == nullptr)
					{
						HBL2_CORE_WARN("Asset at path: {0} and with UUID: {1} has not been registered. Registering it now.", entry.path().string(), assetUUID);

						assetHandle = AssetManager::Instance->RegisterAsset(entry.path());
						packedHandle = assetHandle.Pack();
						asset = AssetManager::Instance->GetAssetMetadata(assetHandle);
					}

					switch (asset->Type)
					{
					case AssetType::Shader:
						ImGui::SetDragDropPayload("Content_Browser_Item_Shader", (void*)(uint32_t*)&packedHandle, sizeof(uint32_t));
						break;
					case AssetType::Texture:
						ImGui::SetDragDropPayload("Content_Browser_Item_Texture", (void*)(uint32_t*)&packedHandle, sizeof(uint32_t));
						break;
					case AssetType::Material:
						ImGui::SetDragDropPayload("Content_Browser_Item_Material", (void*)(uint32_t*)&packedHandle, sizeof(uint32_t));
						break;
					case AssetType::Mesh:
						ImGui::SetDragDropPayload("Content_Browser_Item_Mesh", (void*)(uint32_t*)&packedHandle, sizeof(uint32_t));
						break;
					case AssetType::Scene:
						ImGui::SetDragDropPayload("Content_Browser_Item_Scene", (void*)(uint32_t*)&packedHandle, sizeof(uint32_t));
						break;
					case AssetType::Script:
						ImGui::SetDragDropPayload("Content_Browser_Item_Script", (void*)(uint32_t*)&packedHandle, sizeof(uint32_t));
						break;
					default:
						ImGui::SetDragDropPayload("Content_Browser_Item", (void*)(uint32_t*)&packedHandle, sizeof(uint32_t));
						break;
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

			if (m_OpenDeleteConfirmationWindow && m_AssetToBeDeleted.IsValid())
			{
				// TODO: Handle deletion of folder. (Make folders assets??)

				Asset* assetToBeDeleted = AssetManager::Instance->GetAssetMetadata(m_AssetToBeDeleted);

				ImGui::Begin("Delete Confirmation Window", &m_OpenDeleteConfirmationWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

				ImGui::Text("Are you sure you want to delete the asset: %s.\n\nThis action can not be undone.", assetToBeDeleted->FilePath.string().c_str());

				ImGui::NewLine();

				if (ImGui::Button("OK"))
				{
					AssetManager::Instance->DeleteAsset(m_AssetToBeDeleted, true);
					m_OpenDeleteConfirmationWindow = false;
					m_AssetToBeDeleted = Handle<Asset>();
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel"))
				{
					m_OpenDeleteConfirmationWindow = false;
					m_AssetToBeDeleted = Handle<Asset>();
				}

				ImGui::End();
			}
		}

		void EditorPanelSystem::DrawViewportPanel()
		{
			Context::ViewportSize = { ImGui::GetWindowWidth(), ImGui::GetWindowHeight() };
			Context::ViewportPosition = { ImGui::GetWindowPos().x, ImGui::GetWindowPos().y };

			ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

			if (m_ViewportSize != *(glm::vec2*)&viewportPanelSize)
			{
				m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
				EventDispatcher::Get().Post(ViewportSizeEvent(m_ViewportSize.x, m_ViewportSize.y));

				if (m_ActiveScene != nullptr)
				{
					m_ActiveScene->GetRegistry()
						.view<HBL2::Component::Camera>()
						.each([&](HBL2::Component::Camera& camera)
						{
							if (camera.Enabled)
							{
								camera.AspectRatio = m_ViewportSize.x / m_ViewportSize.y;
							}
						});
				}

				m_Context->GetRegistry()
					.view<Component::EditorCamera, HBL2::Component::Camera>()
					.each([&](Component::EditorCamera& editorCamera, HBL2::Component::Camera& camera)
					{
						if (editorCamera.Enabled)
						{
							camera.AspectRatio = m_ViewportSize.x / m_ViewportSize.y;
						}
					});
			}

			ImGui::Image(HBL2::Renderer::Instance->GetColorAttachment(), ImVec2{m_ViewportSize.x, m_ViewportSize.y}, ImVec2{0, 1}, ImVec2{1, 0});

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Scene"))
				{
					uint32_t sceneAssetHandlePacked = *((uint32_t*)payload->Data);
					Handle<Asset> sceneAssetHandle = Handle<Asset>::UnPack(sceneAssetHandlePacked);
					Asset* sceneAsset = AssetManager::Instance->GetAssetMetadata(sceneAssetHandle);

					if (sceneAsset == nullptr)
					{
						HBL2_WARN("Could not load scene - invalid asset handle.");
						ImGui::EndDragDropTarget();
						return;
					}

					auto path = HBL2::Project::GetAssetFileSystemPath(sceneAsset->FilePath);

					if (path.extension().string() != ".humble")
					{
						HBL2_WARN("Could not load {0} - not a scene file", path.filename().string());
						ImGui::EndDragDropTarget();
						return;
					}

					HBL2::SceneManager::Get().LoadScene(sceneAssetHandle);

					m_EditorScenePath = path;

					ImGui::EndDragDropTarget();
				}
			}

			// Gizmos
			if (m_Context->MainCamera != entt::null && Context::Mode == Mode::Editor)
			{
				auto selectedEntity = HBL2::Component::EditorVisible::SelectedEntity;

				if (!ImGuiRenderer::Instance->Gizmos_IsUsing() && !Input::GetKeyDown(GLFW_MOUSE_BUTTON_2))
				{
					if (Input::GetKeyPress(GLFW_KEY_W))
					{
						m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
					}
					else if (Input::GetKeyPress(GLFW_KEY_E))
					{
						m_GizmoOperation = ImGuizmo::OPERATION::ROTATE;
					}
					else if (Input::GetKeyPress(GLFW_KEY_R))
					{
						m_GizmoOperation = ImGuizmo::OPERATION::SCALE;
					}
					else if (Input::GetKeyPress(GLFW_KEY_Q))
					{
						m_GizmoOperation = ImGuizmo::OPERATION::BOUNDS;
					}
				}

				auto& camera = m_Context->GetComponent<HBL2::Component::Camera>(m_Context->MainCamera);
				ImGuiRenderer::Instance->Gizmos_SetOrthographic(camera.Type == HBL2::Component::Camera::Type::Orthographic);

				// Set window for rendering into.
				ImGuiRenderer::Instance->Gizmos_SetDrawlist();

				// Set viewport size.
				ImGuiRenderer::Instance->Gizmos_SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

				if (selectedEntity != entt::null && m_GizmoOperation != ImGuizmo::OPERATION::BOUNDS)
				{
					bool snap = Input::GetKeyDown(GLFW_KEY_LEFT_CONTROL);
					float snapValue = 0.5f;
					if (m_GizmoOperation == ImGuizmo::OPERATION::ROTATE)
					{
						snapValue = 45.0f;
					}
					glm::vec3 snapValues = { snapValue, snapValue, snapValue };

					float* snapValuesFinal = nullptr;
					if (snap)
					{
						snapValuesFinal = glm::value_ptr(snapValues);
					}

					// Transformation gizmo
					auto& transform = m_ActiveScene->GetComponent<HBL2::Component::Transform>(selectedEntity);
					bool editedTransformation = ImGuiRenderer::Instance->Gizmos_Manipulate(
						glm::value_ptr(camera.View),
						glm::value_ptr(camera.Projection),
						m_GizmoOperation,
						ImGuizmo::MODE::LOCAL,
						glm::value_ptr(transform.LocalMatrix),
						nullptr,
						snapValuesFinal
					);

					if (editedTransformation)
					{
						if (ImGuiRenderer::Instance->Gizmos_IsUsing())
						{
							glm::vec3 newTranslation;
							glm::vec3 newRotation;
							glm::vec3 newScale;
							ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform.LocalMatrix), glm::value_ptr(newTranslation), glm::value_ptr(newRotation), glm::value_ptr(newScale));
							transform.Translation = glm::vec3(newTranslation.x, newTranslation.y, newTranslation.z);
							transform.Rotation += glm::vec3(newRotation.x - transform.Rotation.x, newRotation.y - transform.Rotation.y, newRotation.z - transform.Rotation.z);
							transform.Scale = glm::vec3(newScale.x, newScale.y, newScale.z);
						}
					}
				}

				// Camera gizmo
				float viewManipulateRight = ImGui::GetWindowPos().x + ImGui::GetWindowWidth();
				float viewManipulateTop = ImGui::GetWindowPos().y;

				auto& cameraTransform = m_Context->GetComponent<HBL2::Component::Transform>(m_Context->MainCamera);
				ImGuiRenderer::Instance->Gizmos_ViewManipulate(
					glm::value_ptr(camera.View),
					m_CameraPivotDistance,
					ImVec2(viewManipulateRight - 128, viewManipulateTop),
					ImVec2(128, 128),
					0x10101010
				);

				if (ImGuiRenderer::Instance->Gizmos_IsUsingViewManipulate())
				{
					glm::vec3 newTranslation;
					glm::vec3 newRotation;
					glm::vec3 newScale;
					ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(glm::inverse(camera.View)), glm::value_ptr(newTranslation), glm::value_ptr(newRotation), glm::value_ptr(newScale));
					cameraTransform.Translation = glm::vec3(newTranslation.x, newTranslation.y, newTranslation.z);
					cameraTransform.Rotation += glm::vec3(newRotation.x - cameraTransform.Rotation.x, newRotation.y - cameraTransform.Rotation.y, newRotation.z - cameraTransform.Rotation.z);
					cameraTransform.Scale = glm::vec3(newScale.x, newScale.y, newScale.z);
				}
			}
		}
		
		void EditorPanelSystem::DrawPlayStopPanel()
		{
			static bool isPlaying = false;

			if (ImGui::Button("Play"))
			{
				Context::Mode = Mode::Runtime;

				if (!m_ActiveSceneTemp.IsValid())
				{
					m_ActiveSceneTemp = Context::ActiveScene;
					auto playSceneHandle = ResourceManager::Instance->CreateScene({ .name = m_ActiveScene->GetName() + "(Clone)" });
					Scene* playScene = ResourceManager::Instance->GetScene(playSceneHandle);
					Scene::Copy(m_ActiveScene, playScene);
					SceneManager::Get().LoadScene(playSceneHandle, true);
				}

				isPlaying = true;
			}

			ImGui::SameLine();

			if (ImGui::Button("Stop"))
			{
				Context::Mode = Mode::Editor;

				if (m_ActiveSceneTemp.IsValid())
				{
					SceneManager::Get().LoadScene(m_ActiveSceneTemp);
					m_ActiveSceneTemp = {};
				}

				isPlaying = false;
			}

			ImGui::SameLine();

			if (isPlaying)
			{
				ImGui::Text("Playing ...");
			}
			else
			{
				ImGui::Text("Editing ...");
			}
		}

		void EditorPanelSystem::DrawSystemsPanel()
		{
			if (m_ActiveScene != nullptr)
			{
				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Script"))
					{
						uint32_t scriptAssetHandlePacked = *((uint32_t*)payload->Data);
						Handle<Asset> scriptAssetHandle = Handle<Asset>::UnPack(scriptAssetHandlePacked);
						Asset* scriptAsset = AssetManager::Instance->GetAssetMetadata(scriptAssetHandle);

						if (scriptAsset == nullptr)
						{
							HBL2_WARN("Could not load script - invalid asset handle.");
						}
						else
						{
							Handle<Script> scriptHandle = AssetManager::Instance->GetAsset<Script>(scriptAssetHandle);

							if (scriptHandle.IsValid())
							{
								Script* script = ResourceManager::Instance->GetScript(scriptHandle);

								if (script->Type == ScriptType::SYSTEM)
								{
									NativeScriptUtilities::Get().RegisterSystem(script->Name, m_ActiveScene);
								}
								else
								{
									HBL2_WARN("Could not load script - not a system script.");
								}
							}
							else
							{
								HBL2_WARN("Could not load script - invalid script handle.");
							}
						}

						ImGui::EndDragDropTarget();
					}
				}

				ImGui::TextWrapped(m_ActiveScene->GetName().c_str());
				ImGui::NewLine();
				ImGui::Separator();

				ISystem* systemToBeDeregistered = nullptr;

				for (ISystem* system : m_ActiveScene->GetSystems())
				{
					ImGui::TextWrapped(system->Name.c_str());

					const std::string& name = "Deregister System " + system->Name;

					if (ImGui::BeginPopupContextItem(name.c_str()))
					{
						if (ImGui::MenuItem("Deregister"))
						{
							systemToBeDeregistered = system;
						}
						ImGui::EndPopup();
					}

					ImGui::SameLine(ImGui::GetWindowWidth() - 80.0f);
					switch (system->GetState())
					{
					case SystemState::Play:
						ImGui::Button("Pause", ImVec2(60.f, 20.f));
						if (ImGui::IsItemClicked())
						{
							system->SetState(SystemState::Pause);
						}
						break;
					case SystemState::Pause:
						ImGui::Button("Resume", ImVec2(60.f, 20.f));
						if (ImGui::IsItemClicked())
						{
							system->SetState(SystemState::Play);
						}
						break;
					case SystemState::Idle:
						ImGui::Button("Idling", ImVec2(60.f, 20.f));
						break;
					}
					ImGui::Separator();
				}

				if (systemToBeDeregistered != nullptr)
				{
					m_ActiveScene->DeregisterSystem(systemToBeDeregistered);
				}
			}
		}

		void EditorPanelSystem::DrawTrayPanel()
		{
			if (ImGui::Button("Recompile Scripts"))
			{
				if (Context::Mode == Mode::Runtime)
				{
					HBL2_WARN("Hot reloading is not available yet. Skipping requested recompilation.");
				}
				else
				{
					UnityBuilder::Get().Recompile();
				}
			}
		}
	}
}