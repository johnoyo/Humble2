#include "EditorCameraSystem.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace HBL2Editor
{
	void EditorCameraSystem::OnCreate()
	{
		m_Context = HBL2::Context::ActiveScene;

		m_Context->GetRegistry()
			.group<Component::EditorCamera>(entt::get<HBL2::Component::Camera, HBL2::Component::Transform>)
			.each([&](Component::EditorCamera& editorCamera, HBL2::Component::Camera& camera, HBL2::Component::Transform& transform)
			{
				if (editorCamera.Enabled)
				{
					editorCamera.Distance = transform.Translation.z;
				}
			});
	}

	void EditorCameraSystem::OnUpdate(float ts)
	{
		m_Context = HBL2::Context::ActiveScene;

		m_Context->GetRegistry()
			.group<Component::EditorCamera>(entt::get<HBL2::Component::Camera, HBL2::Component::Transform>)
			.each([&](Component::EditorCamera& editorCamera, HBL2::Component::Camera& camera, HBL2::Component::Transform& transform)
			{
				if (editorCamera.Enabled)
				{
					if (HBL2::Input::GetKeyDown(GLFW_KEY_LEFT_ALT))
					{
						const glm::vec2& mouse{ HBL2::Input::GetMousePosition().x, HBL2::Input::GetMousePosition().y };
						glm::vec2 delta = (mouse - editorCamera.InitialMousePosition) * 0.003f;
						editorCamera.InitialMousePosition = mouse;

						if (HBL2::Input::GetKeyDown(GLFW_MOUSE_BUTTON_MIDDLE))
						{
							// Pan Speed.
							float x = std::min(editorCamera.ViewportSize.x / 1000.0f, 2.4f); // max = 2.4f
							float xSpeed = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

							float y = std::min(editorCamera.ViewportSize.y / 1000.0f, 2.4f); // max = 2.4f
							float ySpeed = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

							editorCamera.FocalPoint += -GetRightDirection(editorCamera.Yaw, editorCamera.Pitch) * delta.x * xSpeed * editorCamera.Distance;
							editorCamera.FocalPoint += GetUpDirection(editorCamera.Yaw, editorCamera.Pitch) * delta.y * ySpeed * editorCamera.Distance;

						}
						else if (HBL2::Input::GetKeyDown(GLFW_MOUSE_BUTTON_LEFT))
						{
							float yawSign = GetUpDirection(editorCamera.Yaw, editorCamera.Pitch).y < 0.f ? -1.0f : 1.0f;
							editorCamera.Yaw += yawSign * delta.x * editorCamera.RotationSpeed;
							editorCamera.Pitch += delta.y * editorCamera.RotationSpeed;
						}
						else if (HBL2::Input::GetKeyDown(GLFW_MOUSE_BUTTON_RIGHT))
						{
							float distance = editorCamera.Distance * 0.5f;
							distance = std::max(distance, 0.0f);
							float speed = distance * distance;
							speed = std::min(speed, 100.0f); // max speed = 100

							editorCamera.Distance -= delta.y * speed;
							if (editorCamera.Distance < 1.0f)
							{
								editorCamera.FocalPoint += GetForwardDirection(editorCamera.Yaw, editorCamera.Pitch);
								editorCamera.Distance = 1.0f;
							}
						}
					}

					transform.Translation = CalculatePosition(editorCamera.FocalPoint, editorCamera.Distance, editorCamera.Yaw, editorCamera.Pitch);
					transform.Rotation = { -editorCamera.Pitch, -editorCamera.Yaw, 0.f };
				}
			});
	}

	glm::quat EditorCameraSystem::GetOrientation(float yaw, float pitch) const
	{
		return glm::quat(glm::vec3(-pitch, -yaw, 0.0f));
	}

	glm::vec3 EditorCameraSystem::GetUpDirection(float yaw, float pitch) const
	{
		return glm::rotate(GetOrientation(yaw, pitch), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 EditorCameraSystem::GetRightDirection(float yaw, float pitch) const
	{
		return glm::rotate(GetOrientation(yaw, pitch), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 EditorCameraSystem::GetForwardDirection(float yaw, float pitch) const
	{
		return glm::rotate(GetOrientation(yaw, pitch), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 EditorCameraSystem::CalculatePosition(glm::vec3 focalPoint, float distance, float yaw, float pitch) const
	{
		return focalPoint - GetForwardDirection(yaw, pitch) * distance;
	}
}
