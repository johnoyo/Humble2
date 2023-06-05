#include "EditorPanelSystem.h"

namespace HBL2Editor
{
	void EditorPanelSystem::OnCreate()
	{
		m_Context = HBL2::Context::ActiveScene;

		{
			// Hierachy panel.
			auto hierachyPanel = m_Context->CreateEntity();
			auto& panel = m_Context->AddComponent<Component::EditorPanel>(hierachyPanel);
			panel.Name = "Hierachy";
			panel.Type = Component::EditorPanel::Panel::Hierachy;
		}

		{
			// Properties panel.
			auto propertiesPanel = m_Context->CreateEntity();
			auto& panel = m_Context->AddComponent<Component::EditorPanel>(propertiesPanel);
			panel.Name = "Properties";
			panel.Type = Component::EditorPanel::Panel::Properties;
		}

		{
			// Menubar panel.
			auto menubarPanel = m_Context->CreateEntity();
			auto& panel = m_Context->AddComponent<Component::EditorPanel>(menubarPanel);
			panel.Name = "Menubar";
			panel.Type = Component::EditorPanel::Panel::Menubar;
			panel.UseBeginEnd = false;
		}

		{
			// Console panel.
			auto consolePanel = m_Context->CreateEntity();
			auto& panel = m_Context->AddComponent<Component::EditorPanel>(consolePanel);
			panel.Name = "Console";
			panel.Type = Component::EditorPanel::Panel::Console;
		}

		{
			// Viewport panel.
			auto viewportPanel = m_Context->CreateEntity();
			auto& panel = m_Context->AddComponent<Component::EditorPanel>(viewportPanel);
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
					case Component::EditorPanel::Panel::Menubar:
						DrawToolBarPanel(m_Context);
						break;
					case Component::EditorPanel::Panel::Console:
						DrawConsolePanel(m_Context, ts);
						break;
					case Component::EditorPanel::Panel::Viewport:
						DrawViewportPanel(m_Context);
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
		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::MenuItem("Create Empty"))
			{
				auto entity = m_Context->CreateEntity();
				context->AddComponent<Component::EditorVisible>(entity);
			}

			if (ImGui::MenuItem("Create Sprite"))
			{
				auto entity = context->CreateEntity();
				context->GetComponent<HBL2::Component::Tag>(entity).Name = "Sprite";
				context->AddComponent<Component::EditorVisible>(entity);
				context->AddComponent<HBL2::Component::Sprite>(entity);
			}

			if (ImGui::MenuItem("Create Camera"))
			{
				auto entity = context->CreateEntity();
				context->GetComponent<HBL2::Component::Tag>(entity).Name = "Camera";
				context->AddComponent<Component::EditorVisible>(entity);
				context->AddComponent<HBL2::Component::Camera>(entity);
			}

			ImGui::EndPopup();
		}

		context->GetRegistry()
			.group<Component::EditorVisible>(entt::get<HBL2::Component::Tag>)
			.each([&](Component::EditorVisible& editorVisible, HBL2::Component::Tag& tag)
			{
				ImGui::Text("%s", tag.Name.c_str());
			});
	}

	void EditorPanelSystem::DrawPropertiesPanel(HBL2::Scene* context)
	{
	}

	void EditorPanelSystem::DrawToolBarPanel(HBL2::Scene* context)
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Close"))
					;//m_Window->Close();
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Edit"))
			{
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
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
			HBL2::Renderer2D::Get().GetFrameBuffer()->Resize((uint32_t)viewportPanelSize.x, (uint32_t)viewportPanelSize.y);
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
							camera.Projection = glm::ortho(-camera.AspectRatio * camera.ZoomLevel, camera.AspectRatio * camera.ZoomLevel, -camera.ZoomLevel, camera.ZoomLevel, -1.f, 1.f);
							camera.ViewProjectionMatrix = camera.Projection * camera.View;
						}
					});
			}
			else if (true)
			{
				context->GetRegistry()
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

		ImGui::Image((void*)HBL2::Renderer2D::Get().GetFrameBuffer()->GetColorAttachmentID(), ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
	}
}