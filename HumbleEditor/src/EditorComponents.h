#pragma once

namespace HBL2Editor
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
				Custom
			} Type = Panel::None;

			bool Enabled = true;
		};

		struct EditorCamera
		{
			float RotationSpeed = 0.5f;
			float Distance = 0.f;
			float Yaw = 0.f;
			float Pitch = 0.f;

			glm::vec3 FocalPoint = { 0.f, 0.f, 0.f };

			glm::vec2 InitialMousePosition = { 0.f, 0.f };
			glm::vec2 ViewportSize = { 0.f, 0.f };

			bool Enabled = true;
		};
	}
}