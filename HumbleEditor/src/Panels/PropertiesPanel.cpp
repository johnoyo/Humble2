#include "Systems\EditorPanelSystem.h"

#include "Script\BuildEngine.h"

#include <UI\UserInterfaceUtilities.h>
#include <UI\LayoutLib.h>
#include <UI\AnimationCurveEditor.h>

namespace HBL2
{
	namespace Editor
	{
		template<typename T>
		static void DrawComponent(const std::string& name, Scene* ctx, std::function<void(T&)> drawUI)
		{
			const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowOverlap;

			if (ctx->HasComponent<T>(HBL2::Component::EditorVisible::SelectedEntity))
			{
				ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
				float lineHeight = GImGui->Font->LegacySize + GImGui->Style.FramePadding.y * 2.0f;
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

		template<typename T>
		static void AddComponentButton(const std::string& name, Scene* ctx)
		{
			if (!ctx->HasComponent<T>(HBL2::Component::EditorVisible::SelectedEntity))
			{
				if (ImGui::MenuItem(name.c_str()))
				{
					ctx->AddComponent<T>(HBL2::Component::EditorVisible::SelectedEntity);
					ImGui::CloseCurrentPopup();
				}
			}
		}

		static void UpdateAssetMetadataFile(Handle<Asset> handle, SceneDescriptor& desc)
		{
			if (!AssetManager::Instance->IsAssetValid(handle))
			{
				return;
			}

			Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
			const std::filesystem::path& filePath = HBL2::Project::GetAssetFileSystemPath(asset->FilePath).string() + ".hblscene";

			std::ofstream fout(filePath, 0);

			YAML::Emitter out;
			out << YAML::BeginMap;
			out << YAML::Key << "Scene" << YAML::Value;
			out << YAML::BeginMap;
			out << YAML::Key << "UUID" << YAML::Value << asset->UUID;
			out << YAML::Key << "MaxEntities" << YAML::Value << desc.maxEntities;
			out << YAML::Key << "MaxComponents" << YAML::Value << desc.maxComponents;
			out << YAML::Key << "MaxSystems" << YAML::Value << desc.maxSystems;
			out << YAML::Key << "MaxJobsPerSystem" << YAML::Value << desc.maxJobsPerSystem;
			out << YAML::Key << "MaxStructuralCommandsPerFramePerThread" << YAML::Value << desc.maxStructuralCommandsPerFramePerThread;
			out << YAML::Key << "UseStructuralCommandBuffer" << YAML::Value << desc.useStructuralCommandBuffer;
			out << YAML::EndMap;
			out << YAML::EndMap;
			fout << out.c_str();
			fout.close();
		}

		void EditorPanelSystem::DrawPropertiesPanel()
		{
			const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowOverlap;

			if (HBL2::Component::EditorVisible::SelectedEntity != Entity::Null)
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
							}

							ImGui::EndDragDropTarget();
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
						ImGui::SliderFloat("Near", &camera.Near, 0, 100);
						ImGui::SliderFloat("Far", &camera.Far, 100, 1500);
						ImGui::SliderFloat("FOV", &camera.Fov, 0, 120);
						ImGui::SliderFloat("Aspect Ratio", &camera.AspectRatio, 0, 3);
						ImGui::SliderFloat("Exposure", &camera.Exposure, 0, 50);
						ImGui::SliderFloat("Gamma", &camera.Gamma, 0, 4);
						ImGui::SliderFloat("Zoom Level", &camera.ZoomLevel, 0, 500);

						// Camera type.
						{
							const char* options[] = { "Perspective", "Orthographic" };
							int currentItem = (int)camera.Type;

							if (ImGui::Combo("Type", &currentItem, options, IM_ARRAYSIZE(options)))
							{
								camera.Type = (HBL2::Component::Camera::EType)currentItem;
							}
						}
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
						}

						ImGui::EndDragDropTarget();
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
						}

						ImGui::EndDragDropTarget();
					}

					ImGui::InputScalar("Material", ImGuiDataType_U32, (void*)(intptr_t*)&materialHandle);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Material"))
						{
							uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
							Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

							mesh.Material = AssetManager::Instance->GetAsset<Material>(assetHandle);
						}

						ImGui::EndDragDropTarget();
					}
				});

				DrawComponent<HBL2::Component::Light>("Light", m_ActiveScene, [this](HBL2::Component::Light& light)
				{
					ImGui::Checkbox("Enabled", &light.Enabled);

					// Light type.
					{
						const char* options[] = { "Directional", "Point", "Spot" };
						int currentItem = (int)light.Type;

						if (ImGui::Combo("Type", &currentItem, options, IM_ARRAYSIZE(options)))
						{
							light.Type = (HBL2::Component::Light::EType)currentItem;
						}
					}

					ImGui::Checkbox("CastsShadows", &light.CastsShadows);
					ImGui::SliderFloat("Intensity", &light.Intensity, 0, 30);

					ImGui::ColorEdit3("Color", glm::value_ptr(light.Color));

					ImGui::SliderFloat("ConstantBias", &light.ConstantBias, 0.0f, 2.5f, "%.3f");
					ImGui::SliderFloat("SlopeBias", &light.SlopeBias, 0.0f, 2.5f, "%.3f");
					ImGui::SliderFloat("NormalOffsetScale", &light.NormalOffsetScale, 0.0f, 0.5f, "%.3f");
					ImGui::SliderFloat("FieldOfView", &light.FieldOfView, 0.0f, 120.0f);

					// Set type back.
					if (light.Type == HBL2::Component::Light::EType::Spot)
					{
						ImGui::SliderFloat("Distance", &light.Distance, 0, 150);
						ImGui::SliderFloat("InnerCutOff", &light.InnerCutOff, 0, 50);
						ImGui::SliderFloat("OuterCutOff", &light.OuterCutOff, 0, 50);
					}
				});

				DrawComponent<HBL2::Component::SkyLight>("SkyLight", m_ActiveScene, [this](HBL2::Component::SkyLight& skyLight)
				{
					ImGui::Checkbox("Enabled", &skyLight.Enabled);

					uint32_t textureHandle = skyLight.EquirectangularMap.Pack();
					
					ImGui::InputScalar("Equirectangular Map", ImGuiDataType_U32, (void*)(intptr_t*)&textureHandle);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Texture"))
						{
							uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
							Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

							if (assetHandle.IsValid())
							{
								Asset* textureAsset = AssetManager::Instance->GetAssetMetadata(assetHandle);

								if (!std::filesystem::exists(HBL2::Project::GetAssetFileSystemPath(textureAsset->FilePath).string() + ".hbltexture"))
								{
									TextureUtilities::Get().CreateAssetMetadataFile(assetHandle);
								}
							}

							skyLight.EquirectangularMap = AssetManager::Instance->GetAsset<Texture>(assetHandle);
						}

						ImGui::EndDragDropTarget();
					}

					if (ImGui::Button("Update"))
					{
						skyLight.Converted = false;
					}
				});

				DrawComponent<HBL2::Component::AudioSource>("AudioSource", m_ActiveScene, [this](HBL2::Component::AudioSource& audioSource)
				{
					uint32_t soundHandle = audioSource.Sound.Pack();

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

								if (!std::filesystem::exists(HBL2::Project::GetAssetFileSystemPath(soundAsset->FilePath).string() + ".hblsound"))
								{
									std::ofstream fout(HBL2::Project::GetAssetFileSystemPath(soundAsset->FilePath).string() + ".hblsound", 0);

									YAML::Emitter out;
									out << YAML::BeginMap;
									out << YAML::Key << "Sound" << YAML::Value;
									out << YAML::BeginMap;
									out << YAML::Key << "UUID" << YAML::Value << soundAsset->UUID;
									out << YAML::EndMap;
									out << YAML::EndMap;
									fout << out.c_str();
									fout.close();	
								}
							}

							audioSource.Sound = AssetManager::Instance->GetAsset<Sound>(assetHandle);
						}

						ImGui::EndDragDropTarget();
					}

					ImGui::DragFloat("Volume", &audioSource.Volume, 0.05f, 0.f, 1.f);
					ImGui::DragFloat("Pitch", &audioSource.Pitch, 0.1f, 0.5f, 2.f);

					// PlaybackState.
					{
						const char* options[] = { "Stopped", "Playing", "Paused" };
						int currentItem = (uint8_t)audioSource.State;

						if (ImGui::Combo("Type", &currentItem, options, IM_ARRAYSIZE(options)))
						{
							audioSource.State = (HBL2::Component::AudioSource::PlaybackState)currentItem;
						}
					}

					static bool loop = false;
					ImGui::Checkbox("Loop", &loop);

					if (loop)
					{
						audioSource.Flags |= HBL2::Component::AudioSource::AudioFlags::Looping;
					}
					else
					{
						audioSource.Flags &= ~HBL2::Component::AudioSource::AudioFlags::Looping;
					}

					static bool spatialised = false;
					ImGui::Checkbox("Spatialised", &spatialised);

					if (spatialised)
					{
						audioSource.Flags |= HBL2::Component::AudioSource::AudioFlags::Spatialised;
					}
					else
					{
						audioSource.Flags &= ~HBL2::Component::AudioSource::AudioFlags::Spatialised;
					}

					if (!loop && !spatialised)
					{
						audioSource.Flags = 0;
					}
				});

				DrawComponent<HBL2::Component::AudioListener>("AudioListener", m_ActiveScene, [this](HBL2::Component::AudioListener& audioListener)
				{
					ImGui::Checkbox("Enabled", &audioListener.Enabled);
				});
				
				DrawComponent<HBL2::Component::Rigidbody2D>("Rigidbody2D", m_ActiveScene, [this](HBL2::Component::Rigidbody2D& rb2d)
				{
					ImGui::Checkbox("Enabled", &rb2d.Enabled);

					// Rigidbody type.
					{
						const char* options[] = { "Static", "Kinematic", "Dynamic" };
						int currentItem = (int)rb2d.Type;

						if (ImGui::Combo("Type", &currentItem, options, IM_ARRAYSIZE(options)))
						{
							rb2d.Type = (Physics::BodyType)currentItem;
						}
					}

					ImGui::Checkbox("FixedRotation", &rb2d.FixedRotation);
				});

				DrawComponent<HBL2::Component::BoxCollider2D>("BoxCollider2D", m_ActiveScene, [this](HBL2::Component::BoxCollider2D& bc2d)
				{
					ImGui::Checkbox("Enabled", &bc2d.Enabled);
					ImGui::Checkbox("Trigger", &bc2d.Trigger);
					ImGui::SliderFloat("Density", &bc2d.Density, 0.0f, 10.0f);
					ImGui::SliderFloat("Friction", &bc2d.Friction, 0.0f, 1.0f);
					ImGui::SliderFloat("Restitution", &bc2d.Restitution, 0.0f, 1.0f);

					if (ImGui::DragFloat2("Size", glm::value_ptr(bc2d.Size)))
					{
						bc2d.Dirty = true;
					}

					if (ImGui::DragFloat2("Offset", glm::value_ptr(bc2d.Offset)))
					{
						bc2d.Dirty = true;
					}
				});

				DrawComponent<HBL2::Component::Rigidbody>("Rigidbody", m_ActiveScene, [this](HBL2::Component::Rigidbody& rb)
				{
					ImGui::Checkbox("Enabled", &rb.Enabled);

					ImGui::SliderFloat("Friction", &rb.Friction, 0.0f, 1.0f);
					ImGui::SliderFloat("GravityFactor", &rb.GravityFactor, 0.0f, 3.0f);

					if (ImGui::Checkbox("Trigger", &rb.Trigger))
					{
						rb.Dirty = true;
					}

					// Rigidbody type.
					{
						const char* options[] = { "Static", "Kinematic", "Dynamic" };
						int currentItem = (int)rb.Type;

						if (ImGui::Combo("Type", &currentItem, options, IM_ARRAYSIZE(options)))
						{
							rb.Type = (Physics::BodyType)currentItem;
						}
					}

					// Rigidbody type.
					{
						const char* options[] = { "Discrete", "Continuos" };
						int currentItem = (int)rb.MotionQuality;

						if (ImGui::Combo("MotionQuality", &currentItem, options, IM_ARRAYSIZE(options)))
						{
							rb.MotionQuality = (HBL2::Component::Rigidbody::EMotionQuality)currentItem;
						}
					}
				});

				DrawComponent<HBL2::Component::BoxCollider>("BoxCollider", m_ActiveScene, [this](HBL2::Component::BoxCollider& bc)
				{
					ImGui::Checkbox("Enabled", &bc.Enabled);
					ImGui::DragFloat3("Size", glm::value_ptr(bc.Size));
				});

				DrawComponent<HBL2::Component::SphereCollider>("SphereCollider", m_ActiveScene, [this](HBL2::Component::SphereCollider& sc)
				{
					ImGui::Checkbox("Enabled", &sc.Enabled);
					ImGui::DragFloat("Radius", &sc.Radius);
				});

				DrawComponent<HBL2::Component::CapsuleCollider>("CapsuleCollider", m_ActiveScene, [this](HBL2::Component::CapsuleCollider& cc)
				{
					ImGui::Checkbox("Enabled", &cc.Enabled);
					ImGui::DragFloat("Height", &cc.Height);
					ImGui::DragFloat("Radius", &cc.Radius);
				});

				DrawComponent<HBL2::Component::TerrainCollider>("TerrainCollider", m_ActiveScene, [this](HBL2::Component::TerrainCollider& tc)
				{
					ImGui::Checkbox("Enabled", &tc.Enabled);
					ImGui::DragInt2("ViewedCoord", glm::value_ptr(tc.ViewedCoord));
				});

				DrawComponent<HBL2::Component::Terrain>("Terrain", m_ActiveScene, [this](HBL2::Component::Terrain& t)
				{
					// Normalisation mode.
					{
						const char* options[] = { "Fixed", "Infinite" };
						int currentItem = (int)t.NormaliseMode;

						if (ImGui::Combo("Type", &currentItem, options, IM_ARRAYSIZE(options)))
						{
							t.NormaliseMode = (HBL2::Component::Terrain::ENormaliseMode)currentItem;
						}
					}

					if (t.NormaliseMode == HBL2::Component::Terrain::ENormaliseMode::LOCAL)
					{
						uint32_t textureHandle = t.HeightMap.Pack();

						ImGui::InputScalar("Height Map", ImGuiDataType_U32, (void*)(intptr_t*)&textureHandle);

						if (ImGui::BeginDragDropTarget())
						{
							if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Texture"))
							{
								uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
								Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

								if (assetHandle.IsValid())
								{
									Asset* textureAsset = AssetManager::Instance->GetAssetMetadata(assetHandle);

									if (!std::filesystem::exists(HBL2::Project::GetAssetFileSystemPath(textureAsset->FilePath).string() + ".hbltexture"))
									{
										TextureUtilities::Get().CreateAssetMetadataFile(assetHandle);
									}
								}

								t.HeightMap = assetHandle;
							}

							ImGui::EndDragDropTarget();
						}

						static int gridSize = 3;

						if (ImGui::SliderInt("Grid Size", &gridSize, 1, 7))
						{
							if (gridSize % 2 == 0)
							{
								gridSize++;
							}

							t.NumberOfChunks = gridSize * gridSize;
						}

						ImGui::Checkbox("Use Falloff Map", &t.UseFalloffMap);
					}

					ImGui::DragInt("Seed", (int*)& t.Seed);
					ImGui::DragFloat("HeightMultiplier", &t.HeightMultiplier);
					ImGui::DragFloat("Scale", &t.Scale);
					ImGui::DragFloat("NoiseScale", &t.NoiseScale);
					ImGui::DragFloat2("Offset", glm::value_ptr(t.Offset));

					uint32_t materialHandle = t.Material.Pack();

					ImGui::InputScalar("Material", ImGuiDataType_U32, (void*)(intptr_t*)&materialHandle);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Material"))
						{
							uint32_t packedAssetHandle = *((uint32_t*)payload->Data);
							Handle<Asset> assetHandle = Handle<Asset>::UnPack(packedAssetHandle);

							t.Material = AssetManager::Instance->GetAsset<Material>(assetHandle);
						}

						ImGui::EndDragDropTarget();
					}

					ImGui::Checkbox("AddColliders", &t.AddColliders);

					{
						bool changed = false;

						if (ImGui::CollapsingHeader("Terrain LOD"))
						{
							ImGui::TextUnformatted("Detail Levels");
							ImGui::Separator();

							// Keep at least one LOD level.
							for (size_t i = 0; i < t.DetailLevels.size(); ++i)
							{
								ImGui::PushID((int)i);

								auto& level = t.DetailLevels[i];

								ImGui::BeginGroup();
								ImGui::Text("LOD %zu", i);

								changed |= ImGui::SliderInt("Mesh LOD", &level.Lod, 0, 5);
								changed |= ImGui::DragFloat("Visible Distance", &level.VisibleDstThreshold, 1.0f, 0.0f, 100000.0f, "%.1f");

								// Remove button for all but the last remaining item.
								if (t.DetailLevels.size() > 1)
								{
									if (ImGui::Button("Remove"))
									{
										t.DetailLevels.erase(t.DetailLevels.begin() + (ptrdiff_t)i);
										changed = true;
										ImGui::EndGroup();
										ImGui::PopID();
										break;
									}
								}

								ImGui::EndGroup();
								ImGui::Separator();
								ImGui::PopID();
							}

							if (t.DetailLevels.size() < 6)
							{
								if (ImGui::Button("Add Detail Level"))
								{
									int32_t nextLod = 0;
									float nextDistance = 200.0f;

									if (!t.DetailLevels.empty())
									{
										nextLod = std::min(t.DetailLevels.back().Lod + 1, 5);
										nextDistance = t.DetailLevels.back().VisibleDstThreshold + 200.0f;
									}

									t.DetailLevels.push_back({
										.Lod = nextLod,
										.VisibleDstThreshold = nextDistance,
									});

									changed = true;
								}
							}

							ImGui::SameLine();
							if (ImGui::Button("Reset Defaults"))
							{
								t.DetailLevels.clear();
								t.DetailLevels.push_back({ .Lod = 0, .VisibleDstThreshold = 200.0f });
								t.DetailLevels.push_back({ .Lod = 2, .VisibleDstThreshold = 400.0f });
								t.DetailLevels.push_back({ .Lod = 5, .VisibleDstThreshold = 600.0f });
								changed = true;
							}

							// Sort by distance.
							if (changed)
							{
								std::sort(t.DetailLevels.begin(), t.DetailLevels.end(), [](const auto& a, const auto& b)
								{
									return a.VisibleDstThreshold < b.VisibleDstThreshold;
								});

								// Clamp LODs and enforce non-decreasing distances.
								for (size_t i = 0; i < t.DetailLevels.size(); ++i)
								{
									t.DetailLevels[i].Lod = std::max(0, t.DetailLevels[i].Lod);

									if (i > 0)
									{
										t.DetailLevels[i].VisibleDstThreshold = std::max(t.DetailLevels[i].VisibleDstThreshold, t.DetailLevels[i - 1].VisibleDstThreshold);
									}
								}

								t.MaxViewDst = t.DetailLevels.back().VisibleDstThreshold;
								t.ChunksVisibleInViewDst = std::max(1, (int32_t)(t.MaxViewDst / t.ChunkSize));
							}

							ImGui::Spacing();
							ImGui::Text("Max View Distance: %.1f", t.MaxViewDst);
							ImGui::Text("Chunks Visible In View Distance: %d", t.ChunksVisibleInViewDst);
						}
					}

					if (ImGui::Button("Regenerate"))
					{
						t.Regenerate = true;
					}
				});

				DrawComponent<HBL2::Component::TerrainChunk>("TerrainChunk", m_ActiveScene, [this](HBL2::Component::TerrainChunk& tc)
				{
					ImGui::Checkbox("Visible", &tc.Visible);
					ImGui::DragInt("LevelOfDetail", (int*)&tc.LevelOfDetail);
				});

				DrawComponent<HBL2::Component::AnimationCurve>("AnimationCurve", m_ActiveScene, [this](HBL2::Component::AnimationCurve& curve)
				{
					if (HBL2::EditorUtilities::Get().HasCustomEditor<HBL2::Component::AnimationCurve>())
					{
						HBL2::EditorUtilities::Get().DrawCustomEditor<HBL2::Component::AnimationCurve, AnimationCurveEditor>(curve);
					}
				});

				DrawComponent<HBL2::Component::PrefabInstance>("PrefabInstance", m_ActiveScene, [this](HBL2::Component::PrefabInstance& pi)
				{
					ImGui::Checkbox("Override From Source", &pi.Override);
				});

				using namespace entt::literals;

				// Iterate over all registered meta types.
				Reflect::ForEachRegisteredType([&](const Reflect::TypeEntry& entry)
				{
					if (entry.hasInRegistry(&m_ActiveScene->GetRegistry(), HBL2::Component::EditorVisible::SelectedEntity))
					{
						std::string componentName = BuildEngine::Instance->CleanComponentNameO3(std::string(entry.typeName));
						ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
						float lineHeight = GImGui->Font->LegacySize + GImGui->Style.FramePadding.y * 2.0f;
						bool opened = ImGui::TreeNodeEx((void*)entry.typeId, treeNodeFlags, componentName.c_str());
						ImGui::PopStyleVar();
						ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);

						bool removeComponent = false;

						if (ImGui::Button("-", ImVec2{ lineHeight, lineHeight }))
						{
							removeComponent = true;
						}

						if (opened)
						{
							Reflect::Any componentMeta = entry.getFromRegistry(&m_ActiveScene->GetRegistry(), HBL2::Component::EditorVisible::SelectedEntity);
							HBL2::EditorUtilities::Get().DrawDefaultEditor(componentMeta);

							ImGui::TreePop();
						}

						ImGui::Separator();

						if (removeComponent)
						{
							entry.removeFromRegistry(&m_ActiveScene->GetRegistry(), HBL2::Component::EditorVisible::SelectedEntity);
						}
					}
				});

				// Add component button.
				if (ImGui::Button("Add Component"))
				{
					ImGui::OpenPopup("AddComponent");
				}

				if (ImGui::BeginPopup("AddComponent"))
				{
					AddComponentButton<HBL2::Component::Sprite>("Sprite", m_ActiveScene);
					AddComponentButton<HBL2::Component::StaticMesh>("StaticMesh", m_ActiveScene);
					AddComponentButton<HBL2::Component::Camera>("Camera", m_ActiveScene);
					AddComponentButton<HBL2::Component::Link>("Link", m_ActiveScene);
					AddComponentButton<HBL2::Component::Light>("Light", m_ActiveScene);
					AddComponentButton<HBL2::Component::SkyLight>("SkyLight", m_ActiveScene);
					AddComponentButton<HBL2::Component::AudioSource>("AudioSource", m_ActiveScene);
					AddComponentButton<HBL2::Component::Rigidbody2D>("Rigidbody2D", m_ActiveScene);
					AddComponentButton<HBL2::Component::BoxCollider2D>("BoxCollider2D", m_ActiveScene);
					AddComponentButton<HBL2::Component::Rigidbody>("Rigidbody", m_ActiveScene);
					AddComponentButton<HBL2::Component::BoxCollider>("BoxCollider", m_ActiveScene);
					AddComponentButton<HBL2::Component::SphereCollider>("SphereCollider", m_ActiveScene);
					AddComponentButton<HBL2::Component::CapsuleCollider>("CapsuleCollider", m_ActiveScene);
					AddComponentButton<HBL2::Component::Terrain>("Terrain", m_ActiveScene);
					AddComponentButton<HBL2::Component::AnimationCurve>("AnimationCurve", m_ActiveScene);

					// Iterate over all registered meta types.
					Reflect::ForEachRegisteredType([&](const Reflect::TypeEntry& entry)
					{
						if (!entry.hasInRegistry(&m_ActiveScene->GetRegistry(), HBL2::Component::EditorVisible::SelectedEntity))
						{
							std::string componentName = BuildEngine::Instance->CleanComponentNameO3(std::string(entry.typeName));
							if (ImGui::MenuItem(componentName.c_str()))
							{
								entry.addToRegistry(&m_ActiveScene->GetRegistry(), HBL2::Component::EditorVisible::SelectedEntity);
								ImGui::CloseCurrentPopup();
							}
						}
					});

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
								int currentItem = mat->VariantHash.blendEnabled ? 1 : 0;

								if (ImGui::Combo("Blend Mode", &currentItem, options, IM_ARRAYSIZE(options)))
								{
									mat->VariantHash.blendEnabled = currentItem == 0 ? false : true;
								}
							}

							// Color output.
							bool colorOutput = mat->VariantHash.colorOutput != 0;
							if (ImGui::Checkbox("Color Output", &colorOutput))
							{
								mat->VariantHash.colorOutput = colorOutput ? 1 : 0;
							}
							
							// Depth enabled.
							bool depthEnabled = mat->VariantHash.depthEnabled != 0;
							if (ImGui::Checkbox("Depth", &depthEnabled))
							{
								mat->VariantHash.depthEnabled = depthEnabled ? 1 : 0;
							}
							
							// Depth test mode.
							{
								const char* options[] = { "Less", "Less Equal", "Greater", "Greater Equal", "Equal", "Not Equal", "Always", "Never" };
								int currentItem = (int)mat->VariantHash.depthCompare;

								if (ImGui::Combo("Depth Test", &currentItem, options, IM_ARRAYSIZE(options)))
								{
									mat->VariantHash.depthCompare = (ShaderDescriptor::RenderPipeline::packed_size)(Compare)currentItem;
								}
							}

							// Depth write mode.
							bool depthWrite = mat->VariantHash.depthWrite != 0;
							if (ImGui::Checkbox("Depth Write", &depthWrite))
							{
								mat->VariantHash.depthWrite = depthWrite ? 1 : 0;
							}

							// Stencil enabled.
							bool stencil = mat->VariantHash.stencilEnabled != 0;
							if (ImGui::Checkbox("Stencil", &stencil))
							{
								mat->VariantHash.stencilEnabled = stencil ? 1 : 0;
							}

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
						}
						break;
					case AssetType::Scene:
						{
							Handle<Scene> handle = AssetManager::Instance->GetAsset<Scene>(m_SelectedAsset);
							Scene* scene = ResourceManager::Instance->GetScene(handle);

							SceneDescriptor& desc = scene->GetDescriptor();

							bool dirty = false;
							dirty |= ImGui::InputInt("MaxEntities", (int*)&desc.maxEntities, 4096);
							dirty |= ImGui::InputInt("MaxComponents", (int*)&desc.maxComponents);
							dirty |= ImGui::InputInt("MaxSystems", (int*)&desc.maxSystems);
							dirty |= ImGui::InputInt("MaxJobsPerSystem", (int*)&desc.maxJobsPerSystem);
							dirty |= ImGui::InputInt("MaxStructuralCommandsPerFramePerThread", (int*)&desc.maxStructuralCommandsPerFramePerThread);
							dirty |= ImGui::Checkbox("UseStructuralCommandBuffer", &desc.useStructuralCommandBuffer);

							if (dirty)
							{
								UpdateAssetMetadataFile(m_SelectedAsset, desc);
							}
							break;
						}
					case AssetType::Prefab:
						{
							Handle<Prefab> handle = AssetManager::Instance->GetAsset<Prefab>(m_SelectedAsset);
							Prefab* prefab = ResourceManager::Instance->GetPrefab(handle);

							PrefabDescriptor& desc = prefab->GetDescriptor();

							bool dirty = false;
							dirty |= ImGui::InputInt("MaxEntities", (int*)&desc.maxEntities, 4096);
							dirty |= ImGui::InputInt("MaxComponents", (int*)&desc.maxComponents);

							if (dirty)
							{
								PrefabUtilities::Get().UpdateMetadataFile(m_SelectedAsset, desc);
							}
							break;
						}
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
