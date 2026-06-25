#include "ViewportPanel.h"

#include "Systems/EditorPanelSystem.h"

namespace HBL2::Editor
{
	ViewportPanel::ViewportPanel(const std::string& name, EditorPanelSystem* owner)
	{
		m_Owner = owner;
		Name = name;
	}

	void ViewportPanel::OnAttach()
	{
	}

	void ViewportPanel::OnCreate()
	{
	}

	void ViewportPanel::OnOpen()
	{
	}

	void ViewportPanel::OnRender(float ts)
	{
		Scene* ctx = m_Owner->m_Context;
		Scene* activeScene = m_Owner->m_ActiveScene;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.f, 0.f });

		ImGui::Begin(Name.c_str(), &m_CloseState);

		Context::ViewportSize = { ImGui::GetWindowWidth(), ImGui::GetWindowHeight() };
		Context::ViewportPosition = { ImGui::GetWindowPos().x, ImGui::GetWindowPos().y };

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

		if (m_Owner->m_ViewportSize != *(glm::vec2*)&viewportPanelSize)
		{
			m_Owner->m_ViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
			EventDispatcher::Get().Post(ViewportSizeEvent(m_Owner->m_ViewportSize.x, m_Owner->m_ViewportSize.y));

			if (activeScene != nullptr)
			{
				activeScene->Filter<HBL2::Component::Camera>()
					.ForEach([&](HBL2::Component::Camera& camera)
					{
						if (camera.Enabled)
						{
							camera.AspectRatio = m_Owner->m_ViewportSize.x / m_Owner->m_ViewportSize.y;
						}
					});
			}

			ctx->Filter<Component::EditorCamera, HBL2::Component::Camera>()
				.ForEach([&](Component::EditorCamera& editorCamera, HBL2::Component::Camera& camera)
				{
					if (editorCamera.Enabled)
					{
						camera.AspectRatio = m_Owner->m_ViewportSize.x / m_Owner->m_ViewportSize.y;
					}
				});
		}

		ImTextureID viewportTexture = (ImTextureID)HBL2::Renderer::Instance->GetColorAttachment();

		if (viewportTexture != 0)
		{
			ImGui::Image(viewportTexture, ImVec2{ m_Owner->m_ViewportSize.x, m_Owner->m_ViewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

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

					if (Context::Mode == Mode::Runtime)
					{
						HBL2_WARN("Could not load {0} - exit play mode and then change scenes.", path.filename().string());
						ImGui::EndDragDropTarget();
						return;
					}

					HBL2::SceneManager::Get().LoadScene(sceneAssetHandle, false);

					m_Owner->m_EditorScenePath = path;
				}

				ImGui::EndDragDropTarget();
			}
		}

		// Gizmos
		if (ctx->MainCamera != Entity::Null && Context::Mode == Mode::Editor)
		{
			auto selectedEntity = HBL2::Component::EditorVisible::SelectedEntity;

			if (!ImGuiRenderer::Instance->Gizmos_IsUsing() && !Input::GetKeyDown(KeyCode::MouseRight))
			{
				if (Input::GetKeyPress(KeyCode::W))
				{
					m_GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
				}
				else if (Input::GetKeyPress(KeyCode::E))
				{
					m_GizmoOperation = ImGuizmo::OPERATION::ROTATE;
				}
				else if (Input::GetKeyPress(KeyCode::R))
				{
					m_GizmoOperation = ImGuizmo::OPERATION::SCALE;
				}
				else if (Input::GetKeyPress(KeyCode::Q))
				{
					m_GizmoOperation = ImGuizmo::OPERATION::BOUNDS;
				}

				if (Input::GetKeyPress(KeyCode::M))
				{
					m_Owner->m_GizmoMode = (m_Owner->m_GizmoMode == ImGuizmo::MODE::LOCAL ? ImGuizmo::MODE::WORLD : ImGuizmo::MODE::LOCAL);
				}
			}

			auto& camera = ctx->GetComponent<HBL2::Component::Camera>(ctx->MainCamera);
			ImGuiRenderer::Instance->Gizmos_SetOrthographic(camera.Type == HBL2::Component::Camera::EType::Orthographic);

			// Set window for rendering into.
			ImGuiRenderer::Instance->Gizmos_SetDrawlist();

			// Set viewport size.
			ImGuiRenderer::Instance->Gizmos_SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

			if (selectedEntity != Entity::Null && m_GizmoOperation != ImGuizmo::OPERATION::BOUNDS)
			{
				bool snap = Input::GetKeyDown(KeyCode::LeftControl);
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
				auto& transform = activeScene->GetComponent<HBL2::Component::Transform>(selectedEntity);
				auto& transformEx = activeScene->GetComponent<HBL2::Component::TransformEx>(selectedEntity);
				auto* link = activeScene->TryGetComponent<HBL2::Component::Link>(selectedEntity);

				glm::mat4 parentW(1.0f);
				if (link != nullptr && link->Parent != 0)
				{
					Entity p = activeScene->FindEntityByUUID(link->Parent);
					if (p != Entity::Null)
					{
						parentW = activeScene->GetComponent<HBL2::Component::Transform>(p).WorldMatrix;
					}
				}

				bool edited = ImGuiRenderer::Instance->Gizmos_Manipulate(
					glm::value_ptr(camera.View),
					glm::value_ptr(camera.Projection),
					m_GizmoOperation,
					m_Owner->m_GizmoMode,
					glm::value_ptr(transform.WorldMatrix),
					nullptr,
					snapValuesFinal
				);

				// NOTE: If we move the gizmo really quickly, the child entities will lag behind a bit.
				//		 Thats because, the gizmo codes runs after the Hierachy system. Consider fixing this!
				if (edited && ImGuiRenderer::Instance->Gizmos_IsUsing())
				{
					// Convert edited world -> local
					glm::mat4 localM = glm::inverse(parentW) * transform.WorldMatrix;

					glm::vec3 lt, lr, ls;
					ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localM), glm::value_ptr(lt), glm::value_ptr(lr), glm::value_ptr(ls));

					transform.Translation = lt;
					transform.Rotation = lr;  // degrees
					transform.Scale = ls;

					// Keep world fields in sync.
					glm::vec3 wt, wr, ws;
					ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transform.WorldMatrix), glm::value_ptr(wt), glm::value_ptr(wr), glm::value_ptr(ws));

					transform.WorldTranslation = wt;
					transform.WorldRotation = wr;
					transform.WorldScale = ws;

					transformEx.PrevWorldTranslation = wt;
					transformEx.PrevWorldRotation = wr;
					transformEx.PrevWorldScale = ws;
				}
			}

			// Camera gizmo
			float viewManipulateRight = ImGui::GetWindowPos().x + ImGui::GetWindowWidth();
			float viewManipulateTop = ImGui::GetWindowPos().y;

			auto& cameraTransform = ctx->GetComponent<HBL2::Component::Transform>(ctx->MainCamera);
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

		ImGui::End();

		ImGui::PopStyleVar();
	}

	void ViewportPanel::OnClose()
	{
	}

	void ViewportPanel::OnDestroy()
	{
	}
}
