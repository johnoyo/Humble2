#pragma once

#include "Base.h"

#include "Renderer/VertexArray.h"

#include <entt.hpp>

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
			glm::vec3 Translation = { 0.f, 0.f, 0.f };
			glm::vec3 Rotation = { 0.f, 0.f, 0.f };
			glm::quat QRotation = glm::quat(Rotation);
			glm::vec3 Scale = { 10.f, 10.f, 10.f };

			glm::mat4 Matrix = glm::mat4(1.f);
		};

		struct Sprite
		{
			glm::vec4 Color = { 1.f, 1.f, 1.f, 1.f };
			int TextureIndex = 0;
			bool Static = false;

			bool Enabled = true;
		};

		struct StaticMesh
		{
			std::vector<Buffer> Data;

			VertexArray* VertexArray;
			std::string Path;
			std::string ShaderName;
			bool Static = false;

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
			//glm::mat4 Projection = glm::ortho(-AspectRatio * ZoomLevel, AspectRatio * ZoomLevel, -ZoomLevel, ZoomLevel, -1.f, 1.f);
			glm::mat4 Projection = glm::perspective(glm::radians(Fov), AspectRatio, Near, Far);
			glm::mat4 ViewProjectionMatrix = glm::mat4(1.f);

			bool Primary = true;
			bool Static = false;

			bool Enabled = false;
		};

		struct EditorVisible
		{
			static inline bool Selected = false;
			static inline entt::entity SelectedEntity = entt::null;

			bool Enabled = true;
		};
	}
}