#include "Systems\EditorPanelSystem.h"

namespace HBL2
{
	namespace Editor
	{
		void EditorPanelSystem::DrawHierachy(Entity entity, const auto& entities)
		{
			const auto& tag = m_ActiveScene->GetComponent<HBL2::Component::Tag>(entity);

			bool selectedEntityCondition = HBL2::Component::EditorVisible::Selected && HBL2::Component::EditorVisible::SelectedEntity == entity;
			bool isPrefab = m_ActiveScene->HasComponent<HBL2::Component::PrefabInstance>(entity);

			ImGuiTreeNodeFlags flags = (selectedEntityCondition ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
			flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

			if (isPrefab)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(IM_COL32(0, 255, 239, 255)));
			}

			bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.Name.c_str());

			if (isPrefab)
			{
				ImGui::PopStyleColor();
			}

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

					// Add Link component to this entity (parent of child entity) if it does not have one.
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
					// Defer the duplication at the end of the function, for now just mark the entity.
					m_EntityToBeDuplicated = entity;
				}

				if (m_ActiveScene->HasComponent<HBL2::Component::PrefabInstance>(entity))
				{
					if (ImGui::MenuItem("Unpack prefab"))
					{
						Prefab::Unpack(entity);
					}
					else if (ImGui::MenuItem("Save prefab"))
					{
						// Clear currently selected entity.
						HBL2::Component::EditorVisible::Selected = false;
						HBL2::Component::EditorVisible::SelectedEntity = Entity::Null;

						Prefab::Save(entity);
					}
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

		void EditorPanelSystem::HandleHierachyPanelDragAndDrop()
		{
			if (ImGui::BeginDragDropTarget())
			{
				// Drag and drop target for instantiating a mesh into the scene.
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Mesh"))
				{
					uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
					Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

					if (!assetHandle.IsValid())
					{
						ImGui::EndDragDropTarget();
						return;
					}

					const std::filesystem::path& metadataPath = HBL2::Project::GetAssetFileSystemPath(AssetManager::Instance->GetAssetMetadata(assetHandle)->FilePath).string() + ".hblmesh";
					
					if (!std::filesystem::exists(metadataPath))
					{
						std::ofstream fout(metadataPath, 0);

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

					Handle<Mesh> meshHandle = AssetManager::Instance->GetAsset<Mesh>(assetHandle);

					if (!meshHandle.IsValid())
					{
						ImGui::EndDragDropTarget();
						return;
					}

					Mesh* mesh = ResourceManager::Instance->GetMesh(meshHandle);

					auto mainMeshEntity = m_ActiveScene->CreateEntity(mesh->DebugName);
					m_ActiveScene->AddComponent<HBL2::Component::EditorVisible>(mainMeshEntity);
					m_ActiveScene->AddComponent<HBL2::Component::Link>(mainMeshEntity);

					uint32_t meshIndex = 0;
					uint32_t subMeshIndex = 0;

					for (auto& meshPart : mesh->Meshes)
					{
						auto meshEntity = m_ActiveScene->CreateEntity(meshPart.DebugName);
						m_ActiveScene->AddComponent<HBL2::Component::EditorVisible>(meshEntity);
						auto& link = m_ActiveScene->AddComponent<HBL2::Component::Link>(meshEntity);

						link.Parent = m_ActiveScene->GetComponent<HBL2::Component::ID>(mainMeshEntity).Identifier;

						for (auto& subMesh : meshPart.SubMeshes)
						{
							auto subMeshEntity = m_ActiveScene->CreateEntity(subMesh.DebugName);
							m_ActiveScene->AddComponent<HBL2::Component::EditorVisible>(subMeshEntity);
							auto& link = m_ActiveScene->AddComponent<HBL2::Component::Link>(subMeshEntity);
							link.Parent = m_ActiveScene->GetComponent<HBL2::Component::ID>(meshEntity).Identifier;

							auto& transform = m_ActiveScene->GetComponent<HBL2::Component::Transform>(subMeshEntity);
							transform.Translation = meshPart.ImportedLocalTransform.translation;
							transform.Rotation = meshPart.ImportedLocalTransform.rotation;
							transform.Scale = meshPart.ImportedLocalTransform.scale;

							auto& staticMesh = m_ActiveScene->AddComponent<HBL2::Component::StaticMesh>(subMeshEntity);
							staticMesh.Mesh = meshHandle;
							staticMesh.MeshIndex = meshIndex;
							staticMesh.SubMeshIndex = subMeshIndex;
							staticMesh.Material = subMesh.EmbededMaterial;

							subMeshIndex++;
						}

						subMeshIndex = 0;
						meshIndex++;
					}

					ImGui::EndDragDropTarget();
				}
				
				// Drag and drop target for instantiating a prefab into the scene.
				else if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Prefab"))
				{
					uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
					Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

					if (!assetHandle.IsValid())
					{
						ImGui::EndDragDropTarget();
						return;
					}

					Prefab::Instantiate(assetHandle);

					ImGui::EndDragDropTarget();
				}
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

				if (ImGui::MenuItem("Create Sprite"))
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

				if (ImGui::MenuItem("Create Tessellated Plane"))
				{
					auto entity = HBL2::EntityPreset::CreateTessellatedPlane();

					HBL2::Component::EditorVisible::SelectedEntity = entity;
					HBL2::Component::EditorVisible::Selected = true;
				}

				if (ImGui::MenuItem("Create Cube"))
				{
					auto entity = HBL2::EntityPreset::CreateCube();

					HBL2::Component::EditorVisible::SelectedEntity = entity;
					HBL2::Component::EditorVisible::Selected = true;
				}

				if (ImGui::MenuItem("Create Sphere"))
				{
					auto entity = HBL2::EntityPreset::CreateSphere();

					HBL2::Component::EditorVisible::SelectedEntity = entity;
					HBL2::Component::EditorVisible::Selected = true;
				}

				if (ImGui::MenuItem("Create Capsule"))
				{
					auto entity = HBL2::EntityPreset::CreateCapsule();

					HBL2::Component::EditorVisible::SelectedEntity = entity;
					HBL2::Component::EditorVisible::Selected = true;
				}

				if (ImGui::MenuItem("Create Cylinder"))
				{
					auto entity = HBL2::EntityPreset::CreateCylinder();

					HBL2::Component::EditorVisible::SelectedEntity = entity;
					HBL2::Component::EditorVisible::Selected = true;
				}

				if (ImGui::MenuItem("Create Torus"))
				{
					auto entity = HBL2::EntityPreset::CreateTorus();

					HBL2::Component::EditorVisible::SelectedEntity = entity;
					HBL2::Component::EditorVisible::Selected = true;
				}

				if (ImGui::MenuItem("Create Light"))
				{
					auto entity = HBL2::EntityPreset::CreateLight();

					HBL2::Component::EditorVisible::SelectedEntity = entity;
					HBL2::Component::EditorVisible::Selected = true;
				}

				if (ImGui::MenuItem("Create SkyLight"))
				{
					auto entity = HBL2::EntityPreset::CreateSkyLight();

					HBL2::Component::EditorVisible::SelectedEntity = entity;
					HBL2::Component::EditorVisible::Selected = true;
				}

				ImGui::EndPopup();
			}

			m_EntityToBeDeleted = Entity::Null;

			const auto& entities = m_ActiveScene->Entities();

			if (ImGui::CollapsingHeader("Entity Hierarchy", ImGuiTreeNodeFlags_DefaultOpen))
			{
				HandleHierachyPanelDragAndDrop();

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

			if (m_EntityToBeDeleted != Entity::Null)
			{
				// Destroy entity and clear entityToBeDeleted value.
				if (m_ActiveScene->HasComponent<HBL2::Component::PrefabInstance>(m_EntityToBeDeleted))
				{
					Prefab::Destroy(m_EntityToBeDeleted);
				}
				else
				{
					m_ActiveScene->DestroyEntity(m_EntityToBeDeleted);
				}

				m_EntityToBeDeleted = Entity::Null;

				// Clear currently selected entity.
				HBL2::Component::EditorVisible::SelectedEntity = Entity::Null;
				HBL2::Component::EditorVisible::Selected = false;
			}

			if (m_EntityToBeDuplicated != Entity::Null)
			{
				// Duplicate entity and clear entityToBeDuplicated value.
				m_ActiveScene->DuplicateEntity(m_EntityToBeDuplicated);
				m_EntityToBeDuplicated = Entity::Null;
			}

			// Clear selection if clicked on empty space inside hierachy panel.
			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
			{
				HBL2::Component::EditorVisible::SelectedEntity = Entity::Null;
				HBL2::Component::EditorVisible::Selected = false;
			}
		}
	}
}
