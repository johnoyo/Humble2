#pragma once

namespace HBL2
{
	namespace Editor
	{
		namespace Component
		{
			struct EditorPanel
			{
				std::string Name = "New Panel";
				ImGuiWindowFlags Flags = 0;
				bool Closeable = false;
				bool UseBeginEnd = true;

				struct Style
				{
					ImGuiStyleVar StyleVar;
					ImVec2 VectorValue;
					float FloatValue;
					bool UseFloat;
				};

				std::vector<Style> Styles;

				enum class Panel
				{
					None,
					Hierachy,
					Properties,
					ContentBrowser,
					Viewport,
					Console,
					Menubar,
					Stats,
					PlayStop,
					Systems,
					Tray,
					Custom
				} Type = Panel::None;

				bool Enabled = true;
			};

			struct EditorCamera
			{
				float Yaw = -90.f;
				float Pitch = 0.f;
				glm::vec3 Front = glm::vec3(0.0f, 0.0f, 1.0f);
				glm::vec3 Right = glm::vec3(1.0f, 0.0f, 0.0f);
				glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
				glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
				float MovementSpeed = 8.0f;
				float MouseSensitivity = 45.0f;
				float MousePreviousPositionX = 0.f;
				float MousePreviousPositionY = 0.f;

				float ZoomSpeed = 2.5f;
				float ScrollZoomSpeed = 50.0f;
				float PanSpeed = 5.0f;

				bool Enabled = true;
			};
		}
	}
}