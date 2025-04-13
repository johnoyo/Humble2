#include "Systems\EditorPanelSystem.h"

#include <Utilities\NativeScriptUtilities.h>

#include <UI\UserInterfaceUtilities.h>
#include <UI\LayoutLib.h>

namespace HBL2
{
	namespace Editor
	{
		template<typename T>
		static void DrawComponent(const std::string& name, Scene* ctx, std::function<void(T&)> drawUI)
		{
			const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap;

			if (ctx->HasComponent<T>(HBL2::Component::EditorVisible::SelectedEntity))
			{
				ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
				bool opened = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
				ImGui::PopStyleVar();
				ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);

				bool removeComponent = false;

				if (ImGui::Button("-", ImVec2{ lineHeight, lineHeight }))
				{
					removeComponent = true;
				}

				if (opened)
				{
					auto& component = ctx->GetComponent<T>(HBL2::Component::EditorVisible::SelectedEntity);

					drawUI(component);

					ImGui::TreePop();
				}

				ImGui::Separator();

				if (removeComponent)
				{
					ctx->RemoveComponent<T>(HBL2::Component::EditorVisible::SelectedEntity);
				}
			}
		}

		void EditorPanelSystem::DrawPropertiesPanel()
		{
			const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowItemOverlap;

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

				DrawComponent<HBL2::Component::Link>("Link", m_ActiveScene, [this](HBL2::Component::Link& link)
				{
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
				});

				DrawComponent<HBL2::Component::Camera>("Camera", m_ActiveScene, [this](HBL2::Component::Camera& camera)
				{
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
				});

				DrawComponent<HBL2::Component::Sprite>("Sprite", m_ActiveScene, [this](HBL2::Component::Sprite& sprite)
				{
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
				});

				DrawComponent<HBL2::Component::StaticMesh>("StaticMesh", m_ActiveScene, [this](HBL2::Component::StaticMesh& mesh)
				{
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
				});

				DrawComponent<HBL2::Component::Light>("Light", m_ActiveScene, [this](HBL2::Component::Light& light)
				{
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
				});

				DrawComponent<HBL2::Component::AudioSource>("AudioSource", m_ActiveScene, [this](HBL2::Component::AudioSource& audioSource)
				{
					uint32_t soundHandle = audioSource.Sound.Pack();

					ImGui::Checkbox("Enabled", &audioSource.Enabled);
					ImGui::InputScalar("Sound", ImGuiDataType_U32, (void*)(intptr_t*)&soundHandle);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Sound"))
						{
							uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
							Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

							if (assetHandle.IsValid())
							{
								Asset* soundAsset = AssetManager::Instance->GetAssetMetadata(assetHandle);

								std::ofstream fout(HBL2::Project::GetAssetFileSystemPath(soundAsset->FilePath).string() + ".hblsound", 0);

								YAML::Emitter out;
								out << YAML::BeginMap;
								out << YAML::Key << "Sound" << YAML::Value;
								out << YAML::BeginMap;
								out << YAML::Key << "UUID" << YAML::Value << soundAsset->UUID;
								out << YAML::Key << "Loop" << YAML::Value << false;
								out << YAML::Key << "StartPaused" << YAML::Value << false;
								out << YAML::EndMap;
								out << YAML::EndMap;
								fout << out.c_str();
								fout.close();
							}

							audioSource.Sound = AssetManager::Instance->GetAsset<Sound>(assetHandle);

							ImGui::EndDragDropTarget();
						}
					}
				});
				
				DrawComponent<HBL2::Component::Rigidbody2D>("Rigidbody2D", m_ActiveScene, [this](HBL2::Component::Rigidbody2D& rb2d)
				{
					ImGui::Checkbox("Enabled", &rb2d.Enabled);

					std::string selectedType = "Static";

					switch (rb2d.Type)
					{
					case HBL2::Component::Rigidbody2D::BodyType::Static: selectedType = "Static"; break;
					case HBL2::Component::Rigidbody2D::BodyType::Dynamic: selectedType = "Dynamic"; break;
					case HBL2::Component::Rigidbody2D::BodyType::Kinematic: selectedType = "Kinematic"; break;
					}

					std::string bodyTypes[3] = { "Static", "Dynamic", "Kinematic"};

					if (ImGui::BeginCombo("Type", selectedType.c_str()))
					{
						for (const auto& type : bodyTypes)
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

					if (selectedType == "Static") rb2d.Type = HBL2::Component::Rigidbody2D::BodyType::Static;
					else if (selectedType == "Dynamic") rb2d.Type = HBL2::Component::Rigidbody2D::BodyType::Dynamic;
					else if (selectedType == "Kinematic") rb2d.Type = HBL2::Component::Rigidbody2D::BodyType::Kinematic;

					ImGui::Checkbox("FixedRotation", &rb2d.FixedRotation);
				});

				DrawComponent<HBL2::Component::BoxCollider2D>("BoxCollider2D", m_ActiveScene, [this](HBL2::Component::BoxCollider2D& bc2d)
				{
					ImGui::Checkbox("Enabled", &bc2d.Enabled);
					ImGui::SliderFloat("Density", &bc2d.Density, 0.0f, 10.0f);
					ImGui::SliderFloat("Friction", &bc2d.Friction, 0.0f, 1.0f);
					ImGui::SliderFloat("Restitution", &bc2d.Restitution, 0.0f, 1.0f);
					ImGui::DragFloat2("Size", glm::value_ptr(bc2d.Size));
					ImGui::DragFloat2("Offset", glm::value_ptr(bc2d.Offset));
				});

				using namespace entt::literals;

				// Iterate over all registered meta types
				for (auto meta_type : entt::resolve(m_ActiveScene->GetMetaContext()))
				{
					std::string componentName = meta_type.second.info().name().data();
					componentName = NativeScriptUtilities::Get().CleanComponentNameO3(componentName);

					if (NativeScriptUtilities::Get().HasComponent(componentName, m_ActiveScene, HBL2::Component::EditorVisible::SelectedEntity))
					{
						ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
						float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
						bool opened = ImGui::TreeNodeEx((void*)meta_type.second.info().hash(), treeNodeFlags, componentName.c_str());
						ImGui::PopStyleVar();
						ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);

						bool removeComponent = false;

						if (ImGui::Button("-", ImVec2{ lineHeight, lineHeight }))
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
					if (!m_ActiveScene->HasComponent<HBL2::Component::Sprite>(HBL2::Component::EditorVisible::SelectedEntity))
					{
						if (ImGui::MenuItem("Sprite"))
						{
							m_ActiveScene->AddComponent<HBL2::Component::Sprite>(HBL2::Component::EditorVisible::SelectedEntity);
							ImGui::CloseCurrentPopup();
						}
					}

					if (!m_ActiveScene->HasComponent<HBL2::Component::StaticMesh>(HBL2::Component::EditorVisible::SelectedEntity))
					{
						if (ImGui::MenuItem("StaticMesh"))
						{
							m_ActiveScene->AddComponent<HBL2::Component::StaticMesh>(HBL2::Component::EditorVisible::SelectedEntity);
							ImGui::CloseCurrentPopup();
						}
					}

					if (!m_ActiveScene->HasComponent<HBL2::Component::Camera>(HBL2::Component::EditorVisible::SelectedEntity))
					{
						if (ImGui::MenuItem("Camera"))
						{
							m_ActiveScene->AddComponent<HBL2::Component::Camera>(HBL2::Component::EditorVisible::SelectedEntity);
							ImGui::CloseCurrentPopup();
						}
					}

					if (!m_ActiveScene->HasComponent<HBL2::Component::Link>(HBL2::Component::EditorVisible::SelectedEntity))
					{
						if (ImGui::MenuItem("Link"))
						{
							m_ActiveScene->AddComponent<HBL2::Component::Link>(HBL2::Component::EditorVisible::SelectedEntity);
							ImGui::CloseCurrentPopup();
						}
					}

					if (!m_ActiveScene->HasComponent<HBL2::Component::Light>(HBL2::Component::EditorVisible::SelectedEntity))
					{
						if (ImGui::MenuItem("Light"))
						{
							m_ActiveScene->AddComponent<HBL2::Component::Light>(HBL2::Component::EditorVisible::SelectedEntity);
							ImGui::CloseCurrentPopup();
						}
					}

					if (!m_ActiveScene->HasComponent<HBL2::Component::AudioSource>(HBL2::Component::EditorVisible::SelectedEntity))
					{
						if (ImGui::MenuItem("AudioSource"))
						{
							m_ActiveScene->AddComponent<HBL2::Component::AudioSource>(HBL2::Component::EditorVisible::SelectedEntity);
							ImGui::CloseCurrentPopup();
						}
					}

					if (!m_ActiveScene->HasComponent<HBL2::Component::Rigidbody2D>(HBL2::Component::EditorVisible::SelectedEntity))
					{
						if (ImGui::MenuItem("Rigidbody2D"))
						{
							m_ActiveScene->AddComponent<HBL2::Component::Rigidbody2D>(HBL2::Component::EditorVisible::SelectedEntity);
							ImGui::CloseCurrentPopup();
						}
					}

					if (!m_ActiveScene->HasComponent<HBL2::Component::BoxCollider2D>(HBL2::Component::EditorVisible::SelectedEntity))
					{
						if (ImGui::MenuItem("BoxCollider2D"))
						{
							m_ActiveScene->AddComponent<HBL2::Component::BoxCollider2D>(HBL2::Component::EditorVisible::SelectedEntity);
							ImGui::CloseCurrentPopup();
						}
					}

					// Iterate over all registered meta types
					for (auto meta_type : entt::resolve(m_ActiveScene->GetMetaContext()))
					{
						std::string componentName = meta_type.second.info().name().data();
						componentName = NativeScriptUtilities::Get().CleanComponentNameO3(componentName);

						if (!HBL2::NativeScriptUtilities::Get().HasComponent(componentName, m_ActiveScene, HBL2::Component::EditorVisible::SelectedEntity))
						{
							if (ImGui::MenuItem(componentName.c_str()))
							{
								HBL2::NativeScriptUtilities::Get().AddComponent(componentName, m_ActiveScene, HBL2::Component::EditorVisible::SelectedEntity);
								ImGui::CloseCurrentPopup();
							}
						}
					}

					ImGui::EndPopup();
				}
			}
			else
			{
				if (m_SelectedAsset.IsValid())
				{
					Asset* asset = AssetManager::Instance->GetAssetMetadata(m_SelectedAsset);

					if (asset == nullptr)
					{
						m_SelectedAsset = {};
						return;
					}

					ImGui::Text(std::format("Asset: {}", asset->DebugName).c_str());
					ImGui::Text(std::format("Asset UUID: {}", asset->UUID).c_str());

					ImGui::NewLine();

					switch (asset->Type)
					{
					case AssetType::Texture:
						{
							static bool flip = false;
							if (ImGui::Checkbox("Flip", &flip))
							{
								TextureUtilities::Get().UpdateAssetMetadataFile(m_SelectedAsset, flip);
							}
						}
						break;
					case AssetType::Shader:
						break;
					case AssetType::Material:
						{
							Handle<Material> handle = AssetManager::Instance->GetAsset<Material>(m_SelectedAsset);
							Material* mat = ResourceManager::Instance->GetMaterial(handle);

							if (mat == nullptr)
							{
								break;
							}

							ImGui::Text(std::format("Name: {}", mat->DebugName).c_str());

							// Blend mode.
							{
								const char* options[] = { "Opaque", "Transparent" };
								int currentItem = mat->VariantDescriptor.blend.enabled ? 1 : 0;

								if (ImGui::Combo("Blend Mode", &currentItem, options, IM_ARRAYSIZE(options)))
								{
									mat->VariantDescriptor.blend.enabled = currentItem == 0 ? false : true;
								}
							}

							// Color output.
							ImGui::Checkbox("Color Output", &mat->VariantDescriptor.blend.colorOutput);
							
							// Depth enabled.
							ImGui::Checkbox("Depth", &mat->VariantDescriptor.depthTest.enabled);
							
							// Depth test mode.
							{
								const char* options[] = { "Less", "Less Equal", "Greater", "Greater Equal", "Equal", "Not Equal", "Always", "Never" };
								int currentItem = (int)mat->VariantDescriptor.depthTest.depthTest;

								if (ImGui::Combo("Depth Test", &currentItem, options, IM_ARRAYSIZE(options)))
								{
									mat->VariantDescriptor.depthTest.depthTest = (Compare)currentItem;
								}
							}

							// Depth write mode.
							ImGui::Checkbox("Depth Write", &mat->VariantDescriptor.depthTest.writeEnabled);

							// Stencil enabled.
							ImGui::Checkbox("Stencil", &mat->VariantDescriptor.depthTest.stencilEnabled);

							ImGui::ColorEdit4("AlbedoColor", glm::value_ptr(mat->AlbedoColor));
							ImGui::InputFloat("Glossiness", &mat->Glossiness, 0.05f);
						}
						break;
					case AssetType::Mesh:
						break;
					case AssetType::Sound:
						{
							Handle<Sound> handle = AssetManager::Instance->GetAsset<Sound>(m_SelectedAsset);
							Sound* sound = ResourceManager::Instance->GetSound(handle);

							if (sound == nullptr)
							{
								break;
							}

							ImGui::Text(std::format("Name: {}", sound->Name).c_str());

							ImGui::Checkbox("Loop", &sound->Loop);
							ImGui::Checkbox("StartPaused", &sound->StartPaused);
						}
						break;
					case AssetType::Scene:
						break;
					case AssetType::Script:
						break;
					default:
						break;
					}

					ImGui::NewLine();

					if (ImGui::Button("Save"))
					{
						AssetManager::Instance->SaveAsset(m_SelectedAsset);
					}
				}
			}
		}
	}
}
