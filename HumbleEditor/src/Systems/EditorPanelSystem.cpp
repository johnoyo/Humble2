#include "EditorPanelSystem.h"

#include "Humble2\Utilities\FileDialogs.h"
#include "EditorCameraSystem.h"

namespace HBL2Editor
{
	void EditorPanelSystem::OnCreate()
	{
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
					case Component::EditorPanel::Panel::Console:
						DrawConsolePanel(m_Context, ts);
						break;
					case Component::EditorPanel::Panel::Viewport:
						DrawViewportPanel(m_Context);
						break;
					case Component::EditorPanel::Panel::Menubar:
						DrawToolBarPanel(m_Context);
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
				mesh.Path = "assets/meshes/monkey_smooth.obj";
				mesh.ShaderName = "BasicMesh";
			}

			if (ImGui::MenuItem("Create Monkeh 2"))
			{
				auto entity = context->CreateEntity();
				context->GetComponent<HBL2::Component::Tag>(entity).Name = "Monkeh2";
				context->GetComponent<HBL2::Component::Transform>(entity).Translation = { 25.f, 0.f, 0.f };
				context->GetComponent<HBL2::Component::Transform>(entity).Rotation = { 0.f, -90.f, 0.f };
				context->GetComponent<HBL2::Component::Transform>(entity).Scale = { 10.f, 10.f, 10.f };
				context->AddComponent<HBL2::Component::EditorVisible>(entity);
				auto& mesh = context->AddComponent<HBL2::Component::StaticMesh>(entity);
				mesh.Path = "assets/meshes/monkey_smooth.obj";
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
					ImGui::InputInt("Texture", &sprite.TextureIndex);
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
					std::filesystem::copy("./assets", "C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\bin\\Release-x86_64\\HumbleApp\\assets", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
				}
				if (ImGui::MenuItem("Build & Run (Windows)"))
				{
					// Build.
					system("\"C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\msbuild.exe\" C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\HumbleGameEngine2.sln /t:HumbleApp /p:Configuration=Release");

					// Copy assets to build folder.
					std::filesystem::copy("./assets", "C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\bin\\Release-x86_64\\HumbleApp\\assets", std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);

					// Run.
					system("C:\\dev\\Graphics\\OpenGL_Projects\\HumbleGameEngine2\\bin\\Release-x86_64\\HumbleApp\\HumbleApp.exe");
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
		ImGui::Text("Frame Time: %f", ts);
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
	}
}