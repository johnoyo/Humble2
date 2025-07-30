#include "EditorCameraSystem.h"

namespace HBL2
{
	namespace Editor
	{
		void EditorCameraSystem::OnCreate()
		{
			m_Context->Group<Component::EditorCamera>(Get<HBL2::Component::Transform>)
				.Each([&](Component::EditorCamera& editorCamera, HBL2::Component::Transform& transform)
				{
					if (editorCamera.Enabled)
					{
						editorCamera.Yaw += transform.Rotation.y;
						editorCamera.Pitch += transform.Rotation.x;

						editorCamera.Pitch = glm::clamp(editorCamera.Pitch, -89.0f, 89.0f);

						glm::vec3 front{};
						front.x = glm::cos(glm::radians(editorCamera.Yaw)) * glm::cos(glm::radians(editorCamera.Pitch));
						front.y = glm::sin(glm::radians(editorCamera.Pitch));
						front.z = glm::sin(glm::radians(-editorCamera.Yaw)) * glm::cos(glm::radians(editorCamera.Pitch));

						editorCamera.Front = -glm::normalize(front);
						editorCamera.Right = -glm::normalize(glm::cross(editorCamera.Front, -editorCamera.WorldUp));
						editorCamera.Up = glm::normalize(glm::cross(editorCamera.Right, editorCamera.Front));
					}
				});

			EventDispatcher::Get().Register<ScrollEvent>([&](const ScrollEvent& e)
			{
				if (m_EditorCamera == nullptr || m_Transform == nullptr)
				{
					return;
				}

				float zoomAmount = (float)e.YOffset * m_EditorCamera->ScrollZoomSpeed;
				m_Transform->Translation += m_EditorCamera->Front * zoomAmount;
			});

			EventDispatcher::Get().Register<CursorPositionEvent>([&](const CursorPositionEvent& e)
			{
				if (m_EditorCamera == nullptr || m_Transform == nullptr)
				{
					return;
				}

				if (Input::GetKeyDown(KeyCode::LeftAlt) && Input::GetKeyDown(KeyCode::MouseRight))
				{
					float dy = (float)e.YPosition - m_EditorCamera->MousePreviousPositionY;

					float zoomAmount = dy * m_EditorCamera->ZoomSpeed;

					m_Transform->Translation += m_EditorCamera->Front * zoomAmount;
				}
				else if (Input::GetKeyDown(KeyCode::LeftAlt) && HBL2::Input::GetKeyDown(KeyCode::MouseMiddle))
				{
					float dx = (float)e.XPosition - m_EditorCamera->MousePreviousPositionX;
					float dy = (float)e.YPosition - m_EditorCamera->MousePreviousPositionY;

					glm::vec3 panRight = m_EditorCamera->Right * -dx * m_EditorCamera->PanSpeed;
					glm::vec3 panUp = m_EditorCamera->Up * dy * m_EditorCamera->PanSpeed;

					m_Transform->Translation += panRight + panUp;
				}
				else if (Input::GetKeyDown(KeyCode::MouseRight))
				{
					float dx = (float)e.XPosition - m_EditorCamera->MousePreviousPositionX;
					float dy = (float)e.YPosition - m_EditorCamera->MousePreviousPositionY;

					dx *= -m_EditorCamera->MouseSensitivity;
					dy *= m_EditorCamera->MouseSensitivity;

					m_EditorCamera->Yaw += dx;
					m_EditorCamera->Pitch += dy;

					m_EditorCamera->Pitch = glm::clamp(m_EditorCamera->Pitch, -89.0f, 89.0f);

					glm::vec3 front{};
					front.x = glm::cos(glm::radians(m_EditorCamera->Yaw)) * glm::cos(glm::radians(m_EditorCamera->Pitch));
					front.y = glm::sin(glm::radians(m_EditorCamera->Pitch));
					front.z = glm::sin(glm::radians(-m_EditorCamera->Yaw)) * glm::cos(glm::radians(m_EditorCamera->Pitch));

					m_EditorCamera->Front = -glm::normalize(front);
					m_EditorCamera->Right = -glm::normalize(glm::cross(m_EditorCamera->Front, -m_EditorCamera->WorldUp));
					m_EditorCamera->Up = glm::normalize(glm::cross(m_EditorCamera->Right, m_EditorCamera->Front));

					m_Transform->Rotation += glm::vec3(-dy, dx, 0.0f);
				}

				m_EditorCamera->MousePreviousPositionX = (float)e.XPosition;
				m_EditorCamera->MousePreviousPositionY = (float)e.YPosition;
			});
		}

		void EditorCameraSystem::OnUpdate(float ts)
		{
			m_Context->Group<Component::EditorCamera>(Get<HBL2::Component::Transform>)
				.Each([&](Component::EditorCamera& editorCamera, HBL2::Component::Transform& transform)
				{
					if (editorCamera.Enabled)
					{
						m_EditorCamera = &editorCamera;
						m_Transform = &transform;

						float velocity = 0.0f;
						
						if (Input::GetKeyDown(KeyCode::MouseRight))
						{
							velocity = editorCamera.MovementSpeed * ts;

							if (Input::GetKeyDown(KeyCode::W))
							{
								transform.Translation += editorCamera.Front * velocity;
							}
							if (Input::GetKeyDown(KeyCode::S))
							{
								transform.Translation -= editorCamera.Front * velocity;
							}
							if (Input::GetKeyDown(KeyCode::D))
							{
								transform.Translation += editorCamera.Right * velocity;
							}
							if (Input::GetKeyDown(KeyCode::A))
							{
								transform.Translation -= editorCamera.Right * velocity;
							}
							if (Input::GetKeyDown(KeyCode::E))
							{
								transform.Translation += editorCamera.Up * velocity;
							}
							if (Input::GetKeyDown(KeyCode::Q))
							{
								transform.Translation -= editorCamera.Up * velocity;
							}
						}
					}
					else
					{
						m_EditorCamera = nullptr;
						m_Transform = nullptr;
					}
				});
		}

		void EditorCameraSystem::OnDestroy()
		{
		}
	}
}
