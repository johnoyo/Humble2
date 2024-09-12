#pragma once

#include "Base.h"

#include "Renderer/VertexArray.h"
#include "Resources\Handle.h"

#include "Asset\Asset.h"

#include <entt.hpp>

namespace HBL2
{
	struct Mesh;
	struct Material;
}

namespace HBL2
{
	namespace Component
	{
		struct Tag
		{
			std::string Name = "Unnamed Entity";
		};

		struct ID
		{
			UUID Identifier = 0;
		};

		struct Transform
		{
			glm::vec3 Translation = { 0.f, 0.f, 0.f };
			glm::vec3 Rotation = { 0.f, 0.f, 0.f };
			glm::quat QRotation = glm::quat(Rotation);
			glm::vec3 Scale = { 1.f, 1.f, 1.f };

			glm::mat4 Matrix = glm::mat4(1.f);
			glm::mat4 WorldMatrix = glm::mat4(1.f);
			bool Static = false;
		};

		struct Link
		{
			UUID parent = 0;
		};

		struct Sprite
		{
			glm::vec4 Color = { 1.f, 1.f, 1.f, 1.f };
			std::string Path = "";
			int TextureIndex = 0;

			bool Enabled = true;
		};

		struct StaticMesh
		{
			std::vector<HBL::Buffer> Data;

			VertexArray* VertexArray;
			std::string Path;
			std::string TexturePath = "";
			int TextureIndex = 0;
			std::string ShaderName;

			bool Enabled = true;
		};

		struct Sprite_New
		{
			Handle<Material> Material;
			bool Enabled = true;
		};

		struct StaticMesh_New
		{
			Handle<Mesh> Mesh;
			Handle<Material> Material;
			bool Enabled = true;
		};

		struct Camera
		{
			enum class Type
			{
				Perspective = 0,
				Orthographic = 1,
			};

			float ZoomLevel = 300.f;
			float Fov = 30.f;
			float Near = 0.1f;
			float Far = 1000.f;
			float AspectRatio = 1.778f;
			Type Type = Type::Perspective;

			glm::mat4 View = glm::mat4(1.f);
			glm::mat4 Projection = glm::mat4(1.f);
			glm::mat4 ViewProjectionMatrix = glm::mat4(1.f);

			bool Primary = true;
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