#include "ContentBrowserPanel.h"

#include "ImGui/ImGuiRenderer.h"
#include "Systems/EditorPanelSystem.h"

#include "Script/BuildEngine.h"
#include "Utilities/YamlUtilities.h"
#include "Utilities/PrefabUtilities.h"
#include "Utilities/FileDialogs.h"
#include "Prefab/PrefabSerializer.h"

namespace HBL2::Editor
{
	ContentBrowserPanel::ContentBrowserPanel(const std::string& name, EditorPanelSystem* owner)
	{
		m_Owner = owner;
		Name = name;
	}

	void ContentBrowserPanel::OnAttach()
	{
	}

	void ContentBrowserPanel::OnCreate()
	{
		for (auto& data : m_ShaderUniformBufferData)
		{
			data.clear();
		}
		std::memset(m_ShaderUniformTextureData.Data(), 0, sizeof(uint32_t) * m_ShaderUniformTextureData.Size());

		m_Owner->m_CurrentDirectory = HBL2::Project::GetAssetDirectory();
	}

	void ContentBrowserPanel::OnOpen()
	{
	}

	void ContentBrowserPanel::OnRender(float ts)
	{
		ImGui::Begin(Name.c_str(), &m_CloseState);

		HandleContentBrowserDragAndDrop();

		// Path bar
		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

		std::filesystem::path tempPath = HBL2::Project::GetProjectDirectory();
		const std::filesystem::path& relativeCurrentDirectory = FileUtils::RelativePath(m_Owner->m_CurrentDirectory, HBL2::Project::GetProjectDirectory());
		for (const auto& part : relativeCurrentDirectory)
		{
			tempPath /= part;
			ImGui::SameLine();
			if (ImGui::Button(part.string().c_str()))
			{
				m_Owner->m_CurrentDirectory = tempPath;
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
						m_Owner->m_CurrentDirectory = entry.path();
					}
					else
					{
						m_Owner->m_CurrentDirectory = entry.path().parent_path();
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

			for (const auto& entry : std::filesystem::directory_iterator(m_Owner->m_CurrentDirectory))
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
						HBL2::Component::EditorVisible::SelectedEntity = Entity::Null;
						m_Owner->m_SelectedAsset = assetHandle;
					}
					else
					{
						HBL2::Component::EditorVisible::SelectedEntity = Entity::Null;
						m_Owner->m_SelectedAsset = {};
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
					else if (extension == ".slang")
					{
						if (ImGui::MenuItem("Recompile"))
						{
							AssetManager::Instance->ReloadAssetAsync<Shader>(assetHandle);
						}
					}
					else if (extension == ".mat")
					{
						if (ImGui::MenuItem("Reload"))
						{
							AssetManager::Instance->ReloadAssetAsync<Material>(assetHandle);
						}
					}
					else if (asset && asset->Type == AssetType::Mesh)
					{
						if (ImGui::MenuItem("Reload"))
						{
							AssetManager::Instance->ReloadAssetAsync<Mesh>(assetHandle);
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

					if (asset == nullptr)
					{
						HBL2_CORE_ERROR("Asset at path: {0} is not a valid asset that supports drag and drop, aborting.", entry.path().string());
					}
					else
					{
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
						case AssetType::Prefab:
							ImGui::SetDragDropPayload("Content_Browser_Item_Prefab", (void*)(uint32_t*)&packedHandle, sizeof(uint32_t));
							break;
						default:
							ImGui::SetDragDropPayload("Content_Browser_Item", (void*)(uint32_t*)&packedHandle, sizeof(uint32_t));
							break;
						}
					}

					ImGui::EndDragDropSource();
				}
				//ImGui::PopStyleColor();

				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					if (entry.is_directory())
					{
						m_Owner->m_CurrentDirectory /= entry.path().filename();
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

		ImGui::End();
	}

	void ContentBrowserPanel::OnClose()
	{
	}

	void ContentBrowserPanel::OnDestroy()
	{
	}

	void ContentBrowserPanel::HandleContentBrowserDragAndDrop()
	{
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity_UUID"))
			{
				Scene* activeScene = m_Owner->m_ActiveScene;

				UUID entityUUID = *((UUID*)payload->Data);
				Entity entity = activeScene->FindEntityByUUID(entityUUID);

				if (entity == Entity::Null)
				{
					HBL2_CORE_ERROR("Unable to create prefab asset, entity is invalid!");
					ImGui::EndDragDropTarget();
					return;
				}

				auto& tag = activeScene->GetComponent<HBL2::Component::Tag>(entity).Name;

				const auto& prefabPath = m_Owner->m_CurrentDirectory / (tag + ".prefab").c_str();
				const auto& relativePath = FileUtils::RelativePath(prefabPath, HBL2::Project::GetAssetDirectory());

				// Create and register asset.
				Handle<Asset> prefabAssetHandle = AssetManager::Instance->CreateAsset({
					.debugName = "prefab-asset",
					.filePath = relativePath,
					.type = AssetType::Prefab,
				});

				// Adds the PrefabEntity component and sets its values.
				PrefabUtilities::Get().ConvertEntityToPrefabPhase0(entity, activeScene);

				// The serialization code will add the PrefabInstance component with the correct values, but in the prefab sub-scene.
				UUID prefabEntityUUID = activeScene->GetComponent<HBL2::Component::ID>(entity).Identifier;
				PrefabUtilities::Get().CreateMetadataFile(prefabAssetHandle, prefabEntityUUID);

				// Create the prefab resource.
				const std::string& prefabName = std::filesystem::path(relativePath).filename().stem().string();
				Asset* asset = AssetManager::Instance->GetAssetMetadata(prefabAssetHandle);

				auto prefabHandle = ResourceManager::Instance->CreatePrefab({
					.debugName = prefabName.c_str(),
					.uuid = asset->UUID,
					.baseEntityUUID = prefabEntityUUID,
					.version = 1,
				});

				Prefab* prefab = ResourceManager::Instance->GetPrefab(prefabHandle);

				// Serialize the prefab source into a file.
				PrefabSerializer serializer(prefab);
				serializer.Serialize(Project::GetAssetFileSystemPath(relativePath));

				// Finish the conversion by adding the PrefabInstance component and setting its values from the prefab source.
				if (!PrefabUtilities::Get().ConvertEntityToPrefabPhase1(entity, prefabAssetHandle, activeScene))
				{
					HBL2_CORE_ERROR("Could not convert entity to prefab!");
					ImGui::EndDragDropTarget();
					return;
				}

				// Set it as the currenlty selected entity.
				HBL2::Component::EditorVisible::SelectedEntity = entity;
			}

			ImGui::EndDragDropTarget();
		}
	}

	void ContentBrowserPanel::DrawDirectoryRecursive(const std::filesystem::path& path)
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
				m_Owner->m_CurrentDirectory = entry.path();
			}

			if (open)
			{
				DrawDirectoryRecursive(entry.path());
				ImGui::TreePop();
			}
		}
	}

	void ContentBrowserPanel::DrawContentBrowserContextMenu()
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

					if (ImGui::MenuItem("Custom"))
					{
						m_SelectedShaderType = 3;
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

					if (ImGui::MenuItem("Lit"))
					{
						m_SelectedMaterialType = 1;
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

			ImGui::InputText("Folder Name", m_FolderNameBuffer.data(), 256);

			ImGui::NewLine();

			if (ImGui::Button("OK"))
			{
				try
				{
					std::filesystem::create_directory(m_Owner->m_CurrentDirectory / m_FolderNameBuffer);
				}
				catch (std::exception& e)
				{
					HBL2_ERROR("New folder creation failed: {0}", e.what());
				}

				m_OpenNewFolderSetupPopup = false;
				m_FolderNameBuffer = "NewFolder";
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel"))
			{
				m_OpenNewFolderSetupPopup = false;
				m_FolderNameBuffer = "NewFolder";
			}

			ImGui::End();
		}

		if (m_OpenSceneSetupPopup)
		{
			ImGui::Begin("New Scene", &m_OpenSceneSetupPopup, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::InputText("Scene Name", m_SceneNameBuffer.data(), 256);

			ImGui::NewLine();

			if (ImGui::Button("OK"))
			{
				auto filepath = m_Owner->m_CurrentDirectory / (m_SceneNameBuffer + ".humble");
				const auto& relativePath = std::filesystem::relative(filepath, HBL2::Project::GetAssetDirectory());

				auto assetHandle = AssetManager::Instance->CreateAsset({
					.debugName = "New Scene",
					.filePath = relativePath,
					.type = AssetType::Scene,
				});

				AssetManager::Instance->SaveAsset(assetHandle);

				HBL2::SceneManager::Get().LoadScene(assetHandle, false);

				m_Owner->m_EditorScenePath = filepath;

				m_OpenSceneSetupPopup = false;
				m_SceneNameBuffer = "NewScene";
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel"))
			{
				m_OpenSceneSetupPopup = false;
				m_SceneNameBuffer = "NewScene";
			}

			ImGui::End();
		}

		if (m_OpenScriptSetupPopup)
		{
			ImGui::Begin("System Setup", &m_OpenScriptSetupPopup, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::InputText("System Name", m_SystemNameBuffer.data(), 256);

			ImGui::NewLine();

			if (ImGui::Button("OK"))
			{
				// Create .h file with placeholder code.
				auto scriptAssetHandle = BuildEngine::Instance->CreateSystemFile(m_Owner->m_CurrentDirectory, m_SystemNameBuffer);

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
				m_SystemNameBuffer = "NewScript";
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel"))
			{
				m_OpenScriptSetupPopup = false;
				m_SystemNameBuffer = "NewScript";
			}

			ImGui::End();
		}

		if (m_OpenComponentSetupPopup)
		{
			ImGui::Begin("Component Setup", &m_OpenComponentSetupPopup, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::InputText("Component Name", m_ComponentNameBuffer.data(), 256);

			ImGui::NewLine();

			if (ImGui::Button("OK"))
			{
				// Create .h file with placeholder code.
				auto scriptAssetHandle = BuildEngine::Instance->CreateComponentFile(m_Owner->m_CurrentDirectory, m_ComponentNameBuffer);

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
				m_ComponentNameBuffer = "NewComponent";
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel"))
			{
				m_OpenComponentSetupPopup = false;
				m_ComponentNameBuffer = "NewComponent";
			}

			ImGui::End();
		}

		if (m_OpenHelperScriptSetupPopup)
		{
			ImGui::Begin("Helper Script Setup", &m_OpenHelperScriptSetupPopup, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::InputText("Script Name", m_ScriptNameBuffer.data(), 256);

			ImGui::NewLine();

			if (ImGui::Button("OK"))
			{
				// Create .h file with placeholder code.
				auto scriptAssetHandle = BuildEngine::Instance->CreateHelperScriptFile(m_Owner->m_CurrentDirectory, m_ScriptNameBuffer);

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
				m_ScriptNameBuffer = "NewHelperScript";
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel"))
			{
				m_OpenHelperScriptSetupPopup = false;
				m_ScriptNameBuffer = "NewHelperScript";
			}

			ImGui::End();
		}

		if (m_OpenShaderSetupPopup)
		{
			ImGui::Begin("Shader Setup", &m_OpenShaderSetupPopup, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::InputText("Shader Name", m_ShaderNameBuffer.data(), 256);

			ImGui::NewLine();

			if (ImGui::Button("OK"))
			{
				const auto& relativePath = std::filesystem::relative(m_Owner->m_CurrentDirectory / (m_ShaderNameBuffer + ".slang"), HBL2::Project::GetAssetDirectory());

				auto shaderAssetHandle = AssetManager::Instance->CreateAsset({
					.debugName = "shader-asset",
					.filePath = relativePath,
					.type = AssetType::Shader,
				});

				std::string shaderSource;

				switch (m_SelectedShaderType)
				{
				case 0:
					shaderSource = ShaderUtilities::Get().ReadFile("assets/shaders/unlit.slang");
					break;
				case 1:
					shaderSource = ShaderUtilities::Get().ReadFile("assets/shaders/blinn-phong.slang");
					break;
				case 2:
					shaderSource = ShaderUtilities::Get().ReadFile("assets/shaders/pbr.slang");
					break;
				}

				std::ofstream fout(m_Owner->m_CurrentDirectory / (m_ShaderNameBuffer + ".slang"), 0);
				fout << shaderSource;
				fout.close();

				if (shaderAssetHandle.IsValid())
				{
					ShaderUtilities::Get().CreateShaderMetadataFile(shaderAssetHandle, m_SelectedShaderType);
				}

				m_OpenShaderSetupPopup = false;
				m_ShaderNameBuffer = "New-Shader";
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel"))
			{
				m_OpenShaderSetupPopup = false;
				m_ShaderNameBuffer = "New-Shader";
			}

			ImGui::End();
		}

		if (m_OpenMaterialSetupPopup)
		{
			ImGui::Begin("Material Setup", &m_OpenMaterialSetupPopup, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

			ImGui::InputText("Material Name", m_MaterialNameBuffer.data(), 256);

			ImGui::NewLine();

			ImGui::InputScalar("Shader", ImGuiDataType_U32, (void*)(intptr_t*)&m_ShaderAssetHandlePacked);

			Handle<Asset> shaderAssetHandle = Handle<Asset>::UnPack(m_ShaderAssetHandlePacked);

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Shader"))
				{
					m_ShaderAssetHandlePacked = *((uint32_t*)payload->Data);
					shaderAssetHandle = Handle<Asset>::UnPack(m_ShaderAssetHandlePacked);

					Asset* shaderAsset = AssetManager::Instance->GetAssetMetadata(shaderAssetHandle);

					const auto& filesystemPath = Project::GetAssetFileSystemPath(shaderAsset->FilePath);
					const std::filesystem::path& shaderPath = std::filesystem::exists(filesystemPath) ? filesystemPath : shaderAsset->FilePath;

					// Reflect Shader.
					m_ShaderReflectionData = ShaderUtilities::Get().Reflect(shaderPath.string());

					// Clear reflection data.
					for (auto& data : m_ShaderUniformBufferData)
					{
						data.clear();
					}
					std::memset(m_ShaderUniformTextureData.Data(), 0, sizeof(uint32_t) * m_ShaderUniformTextureData.Size());
				}

				ImGui::EndDragDropTarget();
			}

			Handle<Shader> shaderHandle = AssetManager::Instance->GetAsset<Shader>(shaderAssetHandle);

			ImGui::NewLine();
			
			// Topology.
			{
				const char* options[] = { "Point List", "Line List", "Line Strip", "Triangle List", "Triangle Strip", "Triangle fan", "Patch List" };
				int currentItem = m_Topology;

				if (ImGui::Combo("Topology", &currentItem, options, IM_ARRAYSIZE(options)))
				{
					m_Topology = (ShaderDescriptor::RenderPipeline::packed_size)(Topology)currentItem;
				}
			}

			// Polygon mode.
			{
				const char* options[] = { "Fill", "Line", "Point" };
				int currentItem = m_PolygonMode;

				if (ImGui::Combo("Polygon Mode", &currentItem, options, IM_ARRAYSIZE(options)))
				{
					m_PolygonMode = (ShaderDescriptor::RenderPipeline::packed_size)(PolygonMode)currentItem;
				}
			}

			// Cull mode.
			{
				const char* options[] = { "None", "Front", "Back", "Front and Back" };
				int currentItem = m_CullMode;

				if (ImGui::Combo("Cull Mode", &currentItem, options, IM_ARRAYSIZE(options)))
				{
					m_CullMode = (ShaderDescriptor::RenderPipeline::packed_size)(CullMode)currentItem;
				}
			}

			// Front face.
			{
				const char* options[] = { "Clockwise", "Counter Clockwise" };
				int currentItem = m_FrontFace;

				if (ImGui::Combo("Front Face", &currentItem, options, IM_ARRAYSIZE(options)))
				{
					m_FrontFace = (ShaderDescriptor::RenderPipeline::packed_size)(FrontFace)currentItem;
				}
			}

			// Blend mode.
			{
				const char* options[] = { "Opaque", "Transparent" };
				int currentItem = m_BlendEnabled ? 1 : 0;

				if (ImGui::Combo("Blend Mode", &currentItem, options, IM_ARRAYSIZE(options)))
				{
					m_BlendEnabled = currentItem == 0 ? false : true;
				}
			}

			// Color output.
			ImGui::Checkbox("Color Output", &m_ColorOutput);

			// Depth enabled.
			ImGui::Checkbox("Depth", &m_DepthEnabled);

			// Depth test mode.
			{
				const char* options[] = { "Less", "Less Equal", "Greater", "Greater Equal", "Equal", "Not Equal", "Always", "Never" };
				int currentItem = m_DepthTest;

				if (ImGui::Combo("Depth Test", &currentItem, options, IM_ARRAYSIZE(options)))
				{
					m_DepthTest = currentItem;
				}
			}

			// Depth write mode.
			ImGui::Checkbox("Depth Write", &m_DepthWriteEnabled);

			// Stencil enabled.
			ImGui::Checkbox("Stencil", &m_StencilEnabled);

			if (shaderAssetHandle.IsValid())
			{
				ImGui::Text("Uniforms:");

				for (const auto& descriptorSet : m_ShaderReflectionData.descriptorSets)
				{
					if (descriptorSet.set != 2)
					{
						continue;
					}

					m_ShaderUniformBufferSize = 0;
					m_ShaderUniformTextureSize = 0;

					for (const auto& b : descriptorSet.bindings)
					{
						if (b.type == ResourceType::UniformBuffer)
						{
							auto& uniformBufferBytes = m_ShaderUniformBufferData[m_ShaderUniformBufferSize++];
							uniformBufferBytes.resize(b.size);

							ImGui::Text(b.name.c_str());

							for (const auto& m : b.members)
							{
								uint8_t* memberPtr = uniformBufferBytes.data() + m.offset;

								switch (m.typeInfo.base)
								{
								case MemberBaseType::Float:
								{
									if (m.typeInfo.isArray)
									{
										const uint32_t stride = m.typeInfo.arrayCount > 0 ? m.size / m.typeInfo.arrayCount : 0;

										for (uint32_t i = 0; i < m.typeInfo.arrayCount; ++i)
										{
											float* f = reinterpret_cast<float*>(memberPtr + i * stride);
											const std::string label = m.name + "[" + std::to_string(i) + "]";

											if (m.typeInfo.cols == 1)
											{
												ImGui::InputFloat(label.c_str(), f, 0.f, 0.f, "%.5f");
											}
											else if (m.typeInfo.cols == 2)
											{
												ImGui::InputFloat2(label.c_str(), f, "%.5f");
											}
											else if (m.typeInfo.cols == 3)
											{
												ImGui::InputFloat3(label.c_str(), f, "%.5f");
											}
											else if (m.typeInfo.cols == 4)
											{
												ImGui::InputFloat4(label.c_str(), f, "%.5f");
											}
										}
									}
									else
									{
										float* f = reinterpret_cast<float*>(memberPtr);

										if (m.typeInfo.cols == 1)
										{
											ImGui::InputFloat(m.name.c_str(), f, 0.f, 0.f, "%.5f");
										}
										else if (m.typeInfo.cols == 2)
										{
											ImGui::InputFloat2(m.name.c_str(), f, "%.5f");
										}
										else if (m.typeInfo.cols == 3)
										{
											ImGui::InputFloat3(m.name.c_str(), f, "%.5f");
										}
										else if (m.typeInfo.cols == 4)
										{
											ImGui::InputFloat4(m.name.c_str(), f, "%.5f");
										}
									}
									break;
								}
								}
							}
						}
						else if (b.type == ResourceType::SampledTexture)
						{
							auto& userMapHandlePacked = m_ShaderUniformTextureData[m_ShaderUniformTextureSize++];

							ImVec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

							if (JobSystem::Get().Busy(m_MaterialTextureLoadingCtx))
							{
								color = { 1.0f, 1.0f, 0.0f, 1.0f };
							}
							else if (userMapHandlePacked != 0 && !JobSystem::Get().Busy(m_MaterialTextureLoadingCtx))
							{
								color = { 0.0f, 1.0f, 0.0f, 1.0f };
							}

							ImGui::PushStyleColor(ImGuiCol_Text, color);

							ImGui::InputScalar(b.name.c_str(), ImGuiDataType_U32, (void*)(intptr_t*)&userMapHandlePacked);

							ImGui::PopStyleColor();

							if (ImGui::BeginDragDropTarget())
							{
								if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item_Texture"))
								{
									userMapHandlePacked = *((uint32_t*)payload->Data);
									Handle<Asset> userMapAssetHandle = Handle<Asset>::UnPack(userMapHandlePacked);

									if (userMapAssetHandle.IsValid())
									{
										TextureUtilities::Get().CreateAssetMetadataFile(userMapAssetHandle);
									}

									auto* task = AssetManager::Instance->GetAssetAsync<Texture>(userMapAssetHandle, &m_MaterialTextureLoadingCtx);
									if (task != nullptr)
									{
										task->ThenOnMainThread([task](Handle<Texture> handle)
										{
											AssetManager::Instance->ReleaseResourceTask(task);
										});
									}
								}

								ImGui::EndDragDropTarget();
							}
						}
					}
				}
			}

			ImGui::NewLine();

			if (ImGui::Button("OK"))
			{
				if (JobSystem::Get().Busy(m_MaterialTextureLoadingCtx))
				{
					HBL2_CORE_WARN("Please wait until the textures have finished loading.");
				}
				else
				{
					const auto& relativePath = std::filesystem::relative(m_Owner->m_CurrentDirectory / (m_MaterialNameBuffer + ".mat"), HBL2::Project::GetAssetDirectory());

					auto materialAssetHandle = AssetManager::Instance->CreateAsset({
						.debugName = "material-asset",
						.filePath = relativePath,
						.type = AssetType::Material,
					});

					ShaderUtilities::Get().CreateMaterialAssetFile(materialAssetHandle, {
						.ShaderAssetHandle = shaderAssetHandle,
						.VariantHash =
						{
							.topology = (ShaderDescriptor::RenderPipeline::packed_size)(Topology)m_Topology,
							.polygonMode = (ShaderDescriptor::RenderPipeline::packed_size)(PolygonMode)m_PolygonMode,
							.cullMode = (ShaderDescriptor::RenderPipeline::packed_size)(CullMode)m_CullMode,
							.frontFace = (ShaderDescriptor::RenderPipeline::packed_size)(FrontFace)m_FrontFace,
							.blendEnabled = m_BlendEnabled,
							.colorOutput = m_ColorOutput,
							.depthEnabled = m_DepthEnabled,
							.depthWrite = m_DepthWriteEnabled,
							.stencilEnabled = m_StencilEnabled,
							.depthCompare = (ShaderDescriptor::RenderPipeline::packed_size)(Compare)m_DepthTest,
						},
						.Buffers = { m_ShaderUniformBufferData.Data(), m_ShaderUniformBufferSize },
						.TextureAssets = { m_ShaderUniformTextureData.Data(), m_ShaderUniformTextureSize },
					});

					if (materialAssetHandle.IsValid())
					{
						ShaderUtilities::Get().CreateMaterialMetadataFile(materialAssetHandle, m_SelectedMaterialType);
					}

					m_OpenMaterialSetupPopup = false;

					ClearMaterialContextMenuData();
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Cancel"))
			{
				m_OpenMaterialSetupPopup = false;

				ClearMaterialContextMenuData();
			}

			ImGui::End();
		}
	}

	void ContentBrowserPanel::ClearMaterialContextMenuData()
	{
		m_ShaderAssetHandlePacked = 0;

		m_MaterialNameBuffer = "New-Material";

		for (auto& data : m_ShaderUniformBufferData)
		{
			data.clear();
		}
		std::memset(m_ShaderUniformTextureData.Data(), 0, sizeof(uint32_t) * m_ShaderUniformTextureData.Size());

		m_Topology = 3;
		m_PolygonMode = 0;
		m_CullMode = 2;
		m_FrontFace = 1;
		m_BlendEnabled = false;
		m_ColorOutput = true;
		m_DepthEnabled = true;
		m_DepthWriteEnabled = false;
		m_DepthTest = 1;
		m_StencilEnabled = true;
	}
}
