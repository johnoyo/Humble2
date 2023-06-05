#pragma once

#include "Base.h"

namespace HBL2
{
	namespace Component
	{
		struct Tag
		{
			std::string Name = "Unnamed Entity";
		};

		struct Transform
		{
			glm::vec3 Position = { 0.f, 0.f, 0.f };
			glm::vec3 Rotation = { 0.f, 0.f, 0.f };
			glm::quat QRotation = glm::quat(Rotation);
			glm::vec3 Scale = { 10.f, 10.f, 10.f };

			glm::mat4 Matrix = glm::mat4(1.f);
		};

		struct Sprite
		{
			glm::vec4 Color = { 1.f, 1.f, 1.f, 1.f };

			bool Enabled = true;
		};

		struct Camera
		{
			float ZoomLevel = 300.f;
			float Fov = 30.f;
			float Near = 0.1f;
			float Far = 1000.f;
			float AspectRatio = 1.778f;

			glm::mat4 View = glm::mat4(1.f);
			glm::mat4 Projection = glm::ortho(-AspectRatio * ZoomLevel, AspectRatio * ZoomLevel, -ZoomLevel, ZoomLevel, -1.f, 1.f);
			glm::mat4 ViewProjectionMatrix = glm::mat4(1.f);

			bool Primary = true;
			bool Static = false;

			bool Enabled = false;
		};
	}
}