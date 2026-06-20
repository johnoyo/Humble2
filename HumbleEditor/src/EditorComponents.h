#pragma once

namespace HBL2
{
	namespace Editor
	{
		namespace Component
		{
			struct EditorCamera
			{
				float Yaw = -90.f;
				float Pitch = 0.f;
				glm::vec3 Front = glm::vec3(0.0f, 0.0f, 1.0f);
				glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
				glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
				glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
				float MovementSpeed = 24.0f;
				float MouseSensitivity = 0.1f;
				float MousePreviousPositionX = 0.f;
				float MousePreviousPositionY = 0.f;

				float ZoomSpeed = 1.0f;
				float ScrollZoomSpeed = 10.0f;
				float PanSpeed = 0.25f;

				bool Enabled = true;
			};
		}
	}
}