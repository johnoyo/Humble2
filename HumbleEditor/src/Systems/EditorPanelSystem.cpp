#include "EditorPanelSystem.h"

#include "Humble2\Utilities\FileDialogs.h"
#include "EditorCameraSystem.h"

namespace HBL2Editor
{
	void EditorPanelSystem::OnCreate()
	{
		m_FileIcon = HBL2::Texture::Load("assets/icons/content_browser/file-1453.png")->GetID();
		m_FolderIcon = HBL2::Texture::Load("assets/icons/content_browser/folder-1437.png")->GetID();
		m_BackIcon = HBL2::Texture::Load("assets/icons/content_browser/curved-arrow-4608.png")->GetID();

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
						DrawHierachyPanel(m_Context);
						break;
					case Component::EditorPanel::Panel::Properties:
						DrawPropertiesPanel(m_Context);
						break;
					case Component::EditorPanel::Panel::Viewport:
						DrawViewportPanel(m_Context);
						break;
					case Component::EditorPanel::Panel::Menubar:
						DrawToolBarPanel(m_Context);
						break;
					case Component::EditorPanel::Panel::Stats:
						DrawStatsPanel(m_Context, ts);
						break;
					case Component::EditorPanel::Panel::ContentBrowser:
						DrawContentBrowserPanel(m_Context);
						break;
					case Component::EditorPanel::Panel::Console:
						DrawConsolePanel(m_Context, ts);
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

	void EditorPanelSystem::DrawHierachyPanel(HBL2::Scene* context)
	{
		// Pop up menu when right clicking on an empty space inside the hierachy panel.
		if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight))
		{
			if (ImGui::MenuItem("Create Empty"))
			{
				auto entity = context->CreateEntity();
				context->AddComponent<HBL2::Component::EditorVisible>(entity);
			}

			if (ImGui::MenuItem("Create Sprite"))
			{
				auto entity = context->CreateEntity();
				context->GetComponent<HBL2::Component::Tag>(entity).Name = "Sprite";
				context->GetComponent<HBL2::Component::Transform>(entity).Translation = { 0.f, 15.f, 0.f };
				context->AddComponent<HBL2::Component::EditorVisible>(entity);
				context->AddComponent<HBL2::Component::Sprite>(entity);
			}

			if (ImGui::MenuItem("Create Camera"))
			{
				auto entity = context->CreateEntity();
				context->GetComponent<HBL2::Component::Tag>(entity).Name = "Camera";
				context->AddComponent<HBL2::Component::EditorVisible>(entity);
				context->AddComponent<HBL2::Component::Camera>(entity).Enabled = true;
				context->GetComponent<HBL2::Component::Transform>(entity).Translation.z = 100.f;
			}

			if (ImGui::MenuItem("Create Monkeh"))
			{
				auto entity = context->CreateEntity();
				context->GetComponent<HBL2::Component::Tag>(entity).Name = "Monkeh";
				context->GetComponent<HBL2::Component::Transform>(entity).Scale = { 5.f, 5.f, 5.f };
				context->AddComponent<HBL2::Component::EditorVisible>(entity);
				auto& mesh = context->AddComponent<HBL2::Component::StaticMesh>(entity);
				mesh.Path = "EmptyProject\\Assets\\Meshes\\monkey_smooth.obj";
				mesh.TexturePath = "";
				mesh.ShaderName = "BasicMesh";
			}

			if (ImGui::MenuItem("Create Lost Empire"))
			{
				auto entity = context->CreateEntity();
				context->GetComponent<HBL2::Component::Tag>(entity).Name = "LostEmpire";
				context->GetComponent<HBL2::Component::Transform>(entity).Scale = { 5.f, 5.f, 5.f };
				context->AddComponent<HBL2::Component::EditorVisible>(entity);
				auto& mesh = context->AddComponent<HBL2::Component::StaticMesh>(entity);
				mesh.Path = "EmptyProject\\Assets\\Meshes\\lost_empire.obj";
				mesh.TexturePath = "EmptyProject\\Assets\\Textures\\lost_empire-RGBA.png";
				mesh.ShaderName = "BasicMesh";
			}

			ImGui::EndPopup();
		}

		entt::entity entityToBeDeleted = entt::null;

		// Iterate over all editor visible entities and draw the to the hierachy panel.
		context->GetRegistry()
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
			context->DestroyEntity(entityToBeDeleted);
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

	void EditorPanelSystem::DrawPropertiesPanel(HBL2::Scene* context)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap;

		if (HBL2::Component::EditorVisible::SelectedEntity != entt::null)
		{
			// Tag component.
			if (context->HasComponent<HBL2::Component::Tag>(HBL2::Component::EditorVisible::SelectedEntity))
			{
				auto& tag = context->GetComponent<HBL2::Component::Tag>(HBL2::Component::EditorVisible::SelectedEntity).Name;

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
			if (context->HasComponent<HBL2::Component::Transform>(HBL2::Component::EditorVisible::SelectedEntity))
			{
				if (ImGui::TreeNodeEx((void*)typeid(HBL2::Component::Transform).hash_code(), treeNodeFlags, "Transform"))
				{
					auto& transform = context->GetComponent<HBL2::Component::Transform>(HBL2::Component::EditorVisible::SelectedEntity);

					ImGui::DragFloat3("Translation", glm::value_ptr(transform.Translation), 0.25f);
					ImGui::DragFloat3("Rotation", glm::value_ptr(transform.Rotation), 0.25f);
					ImGui::DragFloat3("Scale", glm::value_ptr(transform.Scale), 0.25f);

					ImGui::TreePop();
				}

				ImGui::Separator();
			}

			// Camera component.
			if (context->HasComponent<HBL2::Component::Camera>(HBL2::Component::EditorVisible::SelectedEntity))
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
					auto& camera = context->GetComponent<HBL2::Component::Camera>(HBL2::Component::EditorVisible::SelectedEntity);

					ImGui::Checkbox("Enabled", &camera.Enabled);
					ImGui::Checkbox("Static", &camera.Static);
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
					context->RemoveComponent<HBL2::Component::Camera>(HBL2::Component::EditorVisible::SelectedEntity);
				}
			}

			// Sprite component.
			if (context->HasComponent<HBL2::Component::Sprite>(HBL2::Component::EditorVisible::SelectedEntity))
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
					auto& sprite = context->GetComponent<HBL2::Component::Sprite>(HBL2::Component::EditorVisible::SelectedEntity);

					ImGui::Checkbox("Enabled", &sprite.Enabled);
					ImGui::Checkbox("Static", &sprite.Static);
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
					context->RemoveComponent<HBL2::Component::Sprite>(HBL2::Component::EditorVisible::SelectedEntity);
				}
			}

			// StaticMesh component.
			if (context->HasComponent<HBL2::Component::StaticMesh>(HBL2::Component::EditorVisible::SelectedEntity))
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
					auto& mesh = context->GetComponent<HBL2::Component::StaticMesh>(HBL2::Component::EditorVisible::SelectedEntity);

					ImGui::Checkbox("Enabled", &mesh.Enabled);
					ImGui::Checkbox("Static", &mesh.Static);
					ImGui::InputText("Mesh", (char*)mesh.Path.c_str(), 256);

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item"))
						{
							auto path = std::filesystem::relative(HBL2::Project::GetAssetFileSystemPath((const wchar_t*)payload->Data), HBL2::Project::GetProjectDirectory().parent_path());

							if (path.extension().string() == ".obj")
							{
								HBL2::Renderer3D::Get().CleanMesh(mesh);
								mesh.Path = path.string();
								HBL2::Renderer3D::Get().SubmitMesh(context->GetComponent<HBL2::Component::Transform>(HBL2::Component::EditorVisible::SelectedEntity), mesh);
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

								// TODO: Improve this. Update the mesh, do not recreate it.
								HBL2::Renderer3D::Get().CleanMesh(mesh);
								HBL2::Renderer3D::Get().SubmitMesh(context->GetComponent<HBL2::Component::Transform>(HBL2::Component::EditorVisible::SelectedEntity), mesh);
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
					context->RemoveComponent<HBL2::Component::StaticMesh>(HBL2::Component::EditorVisible::SelectedEntity);
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
					context->AddComponent<HBL2::Component::Sprite>(HBL2::Component::EditorVisible::SelectedEntity);
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("StaticMesh"))
				{
					context->AddComponent<HBL2::Component::StaticMesh>(HBL2::Component::EditorVisible::SelectedEntity);
					ImGui::CloseCurrentPopup();
				}

				if (ImGui::MenuItem("Camera"))
				{
					context->AddComponent<HBL2::Component::Camera>(HBL2::Component::EditorVisible::SelectedEntity);
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}
	}

	void EditorPanelSystem::DrawToolBarPanel(HBL2::Scene* context)
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Project"))
				{
					std::string filepath = HBL2::FileDialogs::SaveFile("Humble Project (*.hblproj)\0*.hblproj\0");

					HBL2::Project::Create()->Save(filepath);

					auto& startScenePath = HBL2::Project::GetAssetFileSystemPath(HBL2::Project::GetActive()->GetSpecification().StartingScene);

					HBL2::Project::SaveScene(context, startScenePath);

					HBL2::Project::OpenScene(startScenePath);

					m_Context = HBL2::Context::ActiveScene;
					m_EditorScenePath = filepath;
				}
				if (ImGui::MenuItem("Open Project"))
				{
					std::string filepath = HBL2::FileDialogs::OpenFile("Humble Project (*.hblproj)\0*.hblproj\0");

					if (HBL2::Project::Load(std::filesystem::path(filepath)) != nullptr)
					{
						auto& startingScenePath = HBL2::Project::GetAssetFileSystemPath(HBL2::Project::GetActive()->GetSpecification().StartingScene);

						HBL2::Project::OpenScene(startingScenePath);

						m_Context = HBL2::Context::ActiveScene;
						m_EditorScenePath = filepath;
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

						HBL2::Project::SaveScene(context, filepath);

						m_EditorScenePath = filepath;
					}
					else
					{
						HBL2::Project::SaveScene(context, m_EditorScenePath);
					}
				}
				if (ImGui::MenuItem("Save Scene As"))
				{
					std::string filepath = HBL2::FileDialogs::SaveFile("Humble Scene (*.humble)\0*.humble\0");
					
					HBL2::Project::SaveScene(context, filepath);

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
					// Build.
					system("\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\msbuild.exe\" C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\HumbleGameEngine2.sln /t:HumbleApp /p:Configuration=Release");

					// Copy assets to build folder.
					std::filesystem::copy("./EmptyProject", "C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\bin\\Release-x86_64\\HumbleApp\\EmptyProject", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
				}
				if (ImGui::MenuItem("Build & Run (Windows)"))
				{
					// Build.
					system("\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\msbuild.exe\" C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\HumbleGameEngine2.sln /t:HumbleApp /p:Configuration=Release");

					// Copy assets to build folder.
					std::filesystem::copy("./EmptyProject", "C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\bin\\Release-x86_64\\HumbleApp\\EmptyProject", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

					// Run.
					system("C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\bin\\Release-x86_64\\HumbleApp\\HumbleApp.exe \"EmptyProject\\EmptyProject.hblproj\"");
				}
				if (ImGui::MenuItem("Build (Web)"))
				{
					system("C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\Scripts\\emBuildAll.bat");
				}
				if (ImGui::MenuItem("Close"))
				{
					HBL2::Application::Get().GetWindow()->Close();
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

	void EditorPanelSystem::DrawConsolePanel(HBL2::Scene* context, float ts)
	{
		ImGui::Text("Console");
	}

	void EditorPanelSystem::DrawStatsPanel(HBL2::Scene* context, float ts)
	{
		ImGui::Text("Frame Time: %f", ts);
	}

	void EditorPanelSystem::DrawContentBrowserPanel(HBL2::Scene* context)
	{
		float padding = 16.f;
		float thumbnailSize = 128.f;
		float panelWidth = ImGui::GetContentRegionAvail().x;

		int columnCount = (int)(panelWidth / (padding + thumbnailSize));
		columnCount = columnCount < 1 ? 1 : columnCount;

		ImGui::Columns(columnCount, 0, false);

		if (m_CurrentDirectory != HBL2::Project::GetAssetDirectory())
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::ImageButton((ImTextureID)m_BackIcon, { thumbnailSize, thumbnailSize }, { 0, 1 }, { 1, 0 });
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

			std::string& path = entry.path().string();
			auto relativePath = std::filesystem::relative(entry.path(), HBL2::Project::GetAssetDirectory());

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::ImageButton((ImTextureID)(entry.is_directory() ? m_FolderIcon : m_FileIcon), { thumbnailSize, thumbnailSize }, {0, 1}, {1, 0});

			if (ImGui::BeginDragDropSource())
			{
				const wchar_t* path = relativePath.c_str();
				ImGui::SetDragDropPayload("Content_Browser_Item", path, (wcslen(path) + 1) * sizeof(wchar_t));
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

	void EditorPanelSystem::DrawViewportPanel(HBL2::Scene* context)
	{
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

		if (m_ViewportSize != *(glm::vec2*)&viewportPanelSize)
		{
			HBL2::RenderCommand::FrameBuffer->Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
			m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };

			// TODO: Change this to switch for play and edit mode.
			if (false)
			{
				context->GetRegistry()
					.view<HBL2::Component::Camera>()
					.each([&](HBL2::Component::Camera& camera)
					{
						if (camera.Enabled && camera.Primary)
						{
							camera.AspectRatio = m_ViewportSize.x / m_ViewportSize.y;
							// camera.Projection = glm::ortho(-camera.AspectRatio * camera.ZoomLevel, camera.AspectRatio * camera.ZoomLevel, -camera.ZoomLevel, camera.ZoomLevel, -1.f, 1.f);
							camera.Projection = glm::perspective(glm::radians(camera.Fov), camera.AspectRatio, camera.Near, camera.Far);
							camera.ViewProjectionMatrix = camera.Projection * camera.View;
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
							camera.Projection = glm::perspective(glm::radians(camera.Fov), camera.AspectRatio, camera.Near, camera.Far);
							camera.ViewProjectionMatrix = camera.Projection * camera.View;
						}
					});
			}
		}

		ImGui::Image((void*)HBL2::RenderCommand::FrameBuffer->GetColorAttachmentID(), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Content_Browser_Item"))
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