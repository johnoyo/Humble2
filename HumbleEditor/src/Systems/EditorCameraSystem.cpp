#include "EditorCameraSystem.h"

namespace HBL2
{
	namespace Editor
	{
		void EditorCameraSystem::OnCreate()
		{
			m_Context->GetRegistry()
				.group<Component::EditorCamera>(entt::get<HBL2::Component::Transform>)
				.each([&](Component::EditorCamera& editorCamera, HBL2::Component::Transform& transform)
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

			EventDispatcher::Get().Register("ScrollEvent", [&](const Event& e)
			{
				const ScrollEvent& se = dynamic_cast<const ScrollEvent&>(e);

				if (m_EditorCamera == nullptr || m_Transform == nullptr)
				{
					return;
				}

				float zoomAmount = se.YOffset * m_EditorCamera->ScrollZoomSpeed * m_Timestep;
				m_Transform->Translation += m_EditorCamera->Front * zoomAmount;
			});

			EventDispatcher::Get().Register("CursorPositionEvent", [&](const Event& e)
			{
				const CursorPositionEvent& cps = dynamic_cast<const CursorPositionEvent&>(e);

				if (m_EditorCamera == nullptr || m_Transform == nullptr)
				{
					return;
				}

				if (Input::GetKeyDown(GLFW_KEY_LEFT_ALT) && Input::GetKeyDown(GLFW_MOUSE_BUTTON_2))
				{
					float dy = cps.YPosition - m_EditorCamera->MousePreviousPositionY;

					float zoomAmount = dy * m_EditorCamera->ZoomSpeed * m_Timestep;

					m_Transform->Translation += m_EditorCamera->Front * zoomAmount;
				}
				else if (Input::GetKeyDown(GLFW_KEY_LEFT_ALT) && HBL2::Input::GetKeyDown(GLFW_MOUSE_BUTTON_MIDDLE))
				{
					float dx = cps.XPosition - m_EditorCamera->MousePreviousPositionX;
					float dy = cps.YPosition - m_EditorCamera->MousePreviousPositionY;

					glm::vec3 panRight = m_EditorCamera->Right * -dx * m_EditorCamera->PanSpeed * m_Timestep;
					glm::vec3 panUp = m_EditorCamera->Up * dy * m_EditorCamera->PanSpeed * m_Timestep;

					m_Transform->Translation += panRight + panUp;
				}
				else if (Input::GetKeyDown(GLFW_MOUSE_BUTTON_2))
				{
					float dx = cps.XPosition - m_EditorCamera->MousePreviousPositionX;
					float dy = cps.YPosition - m_EditorCamera->MousePreviousPositionY;

					dx *= -m_EditorCamera->MouseSensitivity * m_Timestep;
					dy *= m_EditorCamera->MouseSensitivity * m_Timestep;

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

				m_EditorCamera->MousePreviousPositionX = cps.XPosition;
				m_EditorCamera->MousePreviousPositionY = cps.YPosition;
			});
		}

		void EditorCameraSystem::OnUpdate(float ts)
		{
			m_Context->GetRegistry()
				.group<Component::EditorCamera>(entt::get<HBL2::Component::Transform>)
				.each([&](Component::EditorCamera& editorCamera, HBL2::Component::Transform& transform)
				{
					if (editorCamera.Enabled)
					{
						m_EditorCamera = &editorCamera;
						m_Transform = &transform;
						m_Timestep = ts;

						float velocity = 0.0f;
						
						if (Input::GetKeyDown(GLFW_MOUSE_BUTTON_2))
						{
							velocity = editorCamera.MovementSpeed * ts;

							if (Input::GetKeyDown(GLFW_KEY_W))
							{
								transform.Translation += editorCamera.Front * velocity;
							}
							if (Input::GetKeyDown(GLFW_KEY_S))
							{
								transform.Translation -= editorCamera.Front * velocity;
							}
							if (Input::GetKeyDown(GLFW_KEY_D))
							{
								transform.Translation += editorCamera.Right * velocity;
							}
							if (Input::GetKeyDown(GLFW_KEY_A))
							{
								transform.Translation -= editorCamera.Right * velocity;
							}
							if (Input::GetKeyDown(GLFW_KEY_E))
							{
								transform.Translation += editorCamera.Up * velocity;
							}
							if (Input::GetKeyDown(GLFW_KEY_Q))
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
