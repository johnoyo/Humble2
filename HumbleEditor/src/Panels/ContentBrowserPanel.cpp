#include "Systems\EditorPanelSystem.h"

#include "Utilities\YamlUtilities.h"
#include <Utilities\FileDialogs.h>

namespace HBL2
{
	namespace Editor
	{
		static ResourceTask<Texture>* s_AlbedoMapTask = nullptr;	
		static ResourceTask<Texture>* s_NormalMapTask = nullptr;	
		static ResourceTask<Texture>* s_MetallicMapTask = nullptr;	
		static ResourceTask<Texture>* s_RoughnessMapTask = nullptr;

		void EditorPanelSystem::DrawContentBrowserPanel()
		{
			// Path bar
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			std::filesystem::path tempPath = HBL2::Project::GetProjectDirectory();
			const std::filesystem::path& relativeCurrentDirectory = FileUtils::RelativePath(m_CurrentDirectory, HBL2::Project::GetProjectDirectory());
			for (const auto& part : relativeCurrentDirectory)
			{
				tempPath /= part;
				ImGui::SameLine();
				if (ImGui::Button(part.string().c_str()))
				{
					m_CurrentDirectory = tempPath;
				}
				ImGui::SameLine();
				ImGui::Text("/");
			}

			// Search bar
			ImGui::SameLine(contentRegionAvailable.x - 200);
			ImGui::SetNextItemWidth(200);
			static char searchQueryBuffer[256] = "";
			ImGui::InputTextWithHint("##Search", "Search...", searchQueryBuffer, 256);
			m_SearchQuery = searchQueryBuffer;

			if (!m_SearchQuery.empty())
			{
				for (const auto& entry : std::filesystem::recursive_directory_iterator(HBL2::Project::GetAssetDirectory()))
				{
					std::string filename = entry.path().filename().string();
					if (filename.find(m_SearchQuery) != std::string::npos)
					{
						if (entry.is_directory())
						{
							m_CurrentDirectory = entry.path();
						}
						else
						{
							m_CurrentDirectory = entry.path().parent_path();
						}
					}
				}
			}

			ImGui::Columns(2, "BrowserColumns", true);
			ImGui::SetColumnWidth(0, ImGui::GetWindowWidth() * 0.2f); // 20% for the sidebar

			// Side bar
			ImGui::BeginChild("##Sidebar", ImVec2(0, 0), true);
			DrawDirectoryRecursive(HBL2::Project::GetAssetDirectory());
			ImGui::EndChild();

			ImGui::NextColumn();

			// File view
			ImGui::BeginChild("##FileView", ImVec2(0, 0), true);

			DrawContentBrowserContextMenu();

			float padding = 16.f;
			float thumbnailSize = 128.f;
			float panelWidth = ImGui::GetContentRegionAvail().x;

			int columnCount = (int)(panelWidth / (padding + thumbnailSize));
			columnCount = columnCount < 1 ? 1 : columnCount;

			if (ImGui::BeginTable("FileGrid", columnCount))
			{
				int id = 0;

				for (const auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
				{
					ImGui::PushID(id++);

					const std::string& path = entry.path().string();
					const auto& relativePath = FileUtils::RelativePath(entry.path(), HBL2::Project::GetAssetDirectory());
					const std::string extension = entry.path().extension().string();

					// Do not show engine metadata files.
					if (extension.find(".hbl") != std::string::npos)
					{
						ImGui::PopID();
						continue;
					}

					//ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

					UUID assetUUID = std::hash<std::string>()(relativePath);
					Handle<Asset> assetHandle = AssetManager::Instance->GetHandleFromUUID(assetUUID);
					Asset* asset = AssetManager::Instance->GetAssetMetadata(assetHandle);

					ImGui::TableNextColumn();

					if (ImGui::Button(entry.path().filename().string().c_str(), { thumbnailSize, thumbnailSize }))
					{
						if (!entry.is_directory())
						{
							HBL2::Component::EditorVisible::SelectedEntity = entt::null;
							m_SelectedAsset = assetHandle;
						}
						else
						{
							HBL2::Component::EditorVisible::SelectedEntity = entt::null;
							m_SelectedAsset = {};
						}
					}

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
						case AssetType::Sound:
							ImGui::SetDragDropPayload("Content_Browser_Item_Sound", (void*)(uint32_t*)&packedHandle, sizeof(uint32_t));
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
					//ImGui::PopStyleColor();

					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
					{
						if (entry.is_directory())
						{
							m_CurrentDirectory /= entry.path().filename();
						}
					}

					ImGui::TextWrapped(entry.path().filename().string().c_str());

					ImGui::PopID();
				}

				ImGui::EndTable();
			}

			ImGui::EndChild();

			if (m_OpenDeleteConfirmationWindow && m_AssetToBeDeleted.IsValid())
			{
				// TODO: Handle deletion of folder. (Make folders assets??)

				ImGui::Begin("Delete Confirmation Window", &m_OpenDeleteConfirmationWindow, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

				ImGui::Text("Are you sure you want to delete this asset.\n\nThis action can not be undone.");

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

		void EditorPanelSystem::DrawDirectoryRecursive(const std::filesystem::path& path)
		{
			if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path))
			{
				return;
			}

			for (const auto& entry : std::filesystem::directory_iterator(path))
			{
				if (!entry.is_directory())
				{
					continue;
				}

				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
				bool open = ImGui::TreeNodeEx(entry.path().filename().string().c_str(), flags);
				if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
				{
					m_CurrentDirectory = entry.path();
				}

				if (open)
				{
					DrawDirectoryRecursive(entry.path());
					ImGui::TreePop();
				}
			}
		}

		void EditorPanelSystem::DrawContentBrowserContextMenu()
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

					if (ImGui::MenuItem("Scene"))
					{
						m_OpenSceneSetupPopup = true;
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

			if (m_OpenSceneSetupPopup)
			{
				ImGui::Begin("New Scene", &m_OpenSceneSetupPopup, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

				static char sceneNameBuffer[256] = "NewScene";
				ImGui::InputText("Scene Name", sceneNameBuffer, 256);

				ImGui::NewLine();

				if (ImGui::Button("OK"))
				{
					auto filepath = m_CurrentDirectory / (std::string(sceneNameBuffer) + ".humble");
					const auto& relativePath = std::filesystem::relative(filepath, HBL2::Project::GetAssetDirectory());

					auto assetHandle = AssetManager::Instance->CreateAsset({
						.debugName = "New Scene",
						.filePath = relativePath,
						.type = AssetType::Scene,
					});

					AssetManager::Instance->SaveAsset(assetHandle);

					HBL2::SceneManager::Get().LoadScene(assetHandle, false);

					m_EditorScenePath = filepath;

					m_OpenSceneSetupPopup = false;
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel"))
				{
					m_OpenSceneSetupPopup = false;
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
					const auto& relativePath = std::filesystem::relative(m_CurrentDirectory / (std::string(shaderNameBuffer) + ".shader"), HBL2::Project::GetAssetDirectory());

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

				static float glossiness = 1.0f;
				ImGui::InputFloat("Glossiness", &glossiness, 0.05f);

				static uint32_t albedoMapHandlePacked = 0;

				if (s_AlbedoMapTask && s_AlbedoMapTask->Finished())
				{
					ImGui::PushStyleColor(ImGuiCol_Text, { 0.0f, 1.0f, 0.0f, 1.0f });
				}
				else if (s_AlbedoMapTask && !s_AlbedoMapTask->Finished())
				{
					ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 1.0f, 0.0f, 1.0f });
				}
				ImGui::InputScalar("AlbedoMap", ImGuiDataType_U32, (void*)(intptr_t*)&albedoMapHandlePacked);
				if (s_AlbedoMapTask)
				{
					ImGui::PopStyleColor();
				}

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

						s_AlbedoMapTask = AssetManager::Instance->GetAssetAsync<Texture>(albedoMapAssetHandle);

						ImGui::EndDragDropTarget();
					}
				}

				Handle<Asset> normalMapAssetHandle;
				Handle<Asset> metallicMapAssetHandle;
				Handle<Asset> roughnessMapAssetHandle;

				if (m_SelectedMaterialType == 2)
				{
					// Normal map
					static uint32_t normalMapHandlePacked = 0;
					if (s_NormalMapTask && s_NormalMapTask->Finished())
					{
						ImGui::PushStyleColor(ImGuiCol_Text, { 0.0f, 1.0f, 0.0f, 1.0f });
					}
					else if (s_NormalMapTask && !s_NormalMapTask->Finished())
					{
						ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 1.0f, 0.0f, 1.0f });
					}
					ImGui::InputScalar("NormalMap", ImGuiDataType_U32, (void*)(intptr_t*)&normalMapHandlePacked);
					if (s_NormalMapTask)
					{
						ImGui::PopStyleColor();
					}

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

							s_NormalMapTask = AssetManager::Instance->GetAssetAsync<Texture>(normalMapAssetHandle);

							ImGui::EndDragDropTarget();
						}
					}

					// Metalicness map
					static uint32_t metallicMapHandlePacked = 0;
					if (s_MetallicMapTask && s_MetallicMapTask->Finished())
					{
						ImGui::PushStyleColor(ImGuiCol_Text, { 0.0f, 1.0f, 0.0f, 1.0f });
					}
					else if (s_MetallicMapTask && !s_MetallicMapTask->Finished())
					{
						ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 1.0f, 0.0f, 1.0f });
					}
					ImGui::InputScalar("MetallicMap", ImGuiDataType_U32, (void*)(intptr_t*)&metallicMapHandlePacked);
					if (s_MetallicMapTask)
					{
						ImGui::PopStyleColor();
					}

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

							s_MetallicMapTask = AssetManager::Instance->GetAssetAsync<Texture>(metallicMapAssetHandle);

							ImGui::EndDragDropTarget();
						}
					}

					// Roughness map
					static uint32_t roughnessMapHandlePacked = 0;
					if (s_RoughnessMapTask && s_RoughnessMapTask->Finished())
					{
						ImGui::PushStyleColor(ImGuiCol_Text, { 0.0f, 1.0f, 0.0f, 1.0f });
					}
					else if (s_RoughnessMapTask && !s_RoughnessMapTask->Finished())
					{
						ImGui::PushStyleColor(ImGuiCol_Text, { 1.0f, 1.0f, 0.0f, 1.0f });
					}
					ImGui::InputScalar("RoughnessMap", ImGuiDataType_U32, (void*)(intptr_t*)&roughnessMapHandlePacked);
					if (s_RoughnessMapTask)
					{
						ImGui::PopStyleColor();
					}

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

							s_RoughnessMapTask = AssetManager::Instance->GetAssetAsync<Texture>(roughnessMapAssetHandle);

							ImGui::EndDragDropTarget();
						}
					}
				}

				ImGui::NewLine();

				if (ImGui::Button("OK"))
				{
					if (!shaderAssetHandle.IsValid())
					{
						HBL2_CORE_WARN("Shader field cannot be left blank. Please select the shader you want to use in your material.");
					}
					else if ((s_AlbedoMapTask != nullptr && !s_AlbedoMapTask->Finished()) ||
						(s_NormalMapTask != nullptr && !s_NormalMapTask->Finished()) ||
						(s_MetallicMapTask != nullptr && !s_MetallicMapTask->Finished()) ||
						(s_RoughnessMapTask != nullptr && !s_RoughnessMapTask->Finished()))
					{
						HBL2_CORE_WARN("Please wait until the textures have finished loading.");
					}
					else
					{
						const auto& relativePath = std::filesystem::relative(m_CurrentDirectory / (std::string(materialNameBuffer) + ".mat"), HBL2::Project::GetAssetDirectory());

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
						out << YAML::Key << "Glossiness" << YAML::Value << glossiness;

						if (s_AlbedoMapTask && s_AlbedoMapTask->ResourceHandle.IsValid())
						{
							out << YAML::Key << "AlbedoMap" << YAML::Value << AssetManager::Instance->GetAssetMetadata(albedoMapAssetHandle)->UUID;
							delete s_AlbedoMapTask;
							s_AlbedoMapTask = nullptr;
						}
						else
						{
							out << YAML::Key << "AlbedoMap" << YAML::Value << (UUID)0;
						}

						if (s_NormalMapTask && s_NormalMapTask->ResourceHandle.IsValid())
						{
							out << YAML::Key << "NormalMap" << YAML::Value << AssetManager::Instance->GetAssetMetadata(normalMapAssetHandle)->UUID;
							delete s_NormalMapTask;
							s_NormalMapTask = nullptr;
						}
						else
						{
							out << YAML::Key << "NormalMap" << YAML::Value << (UUID)0;
						}

						if (s_MetallicMapTask && s_MetallicMapTask->ResourceHandle.IsValid())
						{
							out << YAML::Key << "MetallicMap" << YAML::Value << AssetManager::Instance->GetAssetMetadata(metallicMapAssetHandle)->UUID;
							delete s_MetallicMapTask;
							s_MetallicMapTask = nullptr;
						}
						else
						{
							out << YAML::Key << "MetallicMap" << YAML::Value << (UUID)0;
						}

						if (s_RoughnessMapTask && s_RoughnessMapTask->ResourceHandle.IsValid())
						{
							out << YAML::Key << "RoughnessMap" << YAML::Value << AssetManager::Instance->GetAssetMetadata(roughnessMapAssetHandle)->UUID;
							delete s_RoughnessMapTask;
							s_RoughnessMapTask = nullptr;
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
				}

				ImGui::SameLine();

				if (ImGui::Button("Cancel"))
				{
					m_OpenMaterialSetupPopup = false;

					if (s_AlbedoMapTask)
					{
						delete s_AlbedoMapTask;
						s_AlbedoMapTask = nullptr;
					}

					if (s_NormalMapTask)
					{
						delete s_NormalMapTask;
						s_NormalMapTask = nullptr;
					}

					if (s_MetallicMapTask)
					{
						delete s_MetallicMapTask;
						s_MetallicMapTask = nullptr;
					}

					if (s_RoughnessMapTask)
					{
						delete s_RoughnessMapTask;
						s_RoughnessMapTask = nullptr;
					}
				}

				ImGui::End();
			}
		}
	}
}