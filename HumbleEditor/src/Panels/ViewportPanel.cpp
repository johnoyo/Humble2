#include "Systems\EditorPanelSystem.h"

namespace HBL2
{
	namespace Editor
	{
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

			ImGui::Image(HBL2::Renderer::Instance->GetColorAttachment(), ImVec2{ m_ViewportSize.x, m_ViewportSize.y });// , ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

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

					HBL2::SceneManager::Get().LoadScene(sceneAssetHandle, false);

					m_EditorScenePath = path;

					ImGui::EndDragDropTarget();
				}
			}

			// Gizmos
			if (m_Context->MainCamera != entt::null && Context::Mode == Mode::Editor)
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
				}

				auto& camera = m_Context->GetComponent<HBL2::Component::Camera>(m_Context->MainCamera);
				ImGuiRenderer::Instance->Gizmos_SetOrthographic(camera.Type == HBL2::Component::Camera::Type::Orthographic);

				// Set window for rendering into.
				ImGuiRenderer::Instance->Gizmos_SetDrawlist();

				// Set viewport size.
				ImGuiRenderer::Instance->Gizmos_SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

				if (selectedEntity != entt::null && m_GizmoOperation != ImGuizmo::OPERATION::BOUNDS)
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
	}
}