#pragma once

#include "Base.h"

#include "Resources\Handle.h"
#include "Asset\Asset.h"

#include <entt.hpp>

namespace HBL2
{
	struct Mesh;
	struct Material;
	struct Sound;

	namespace Component
	{
		struct HBL2_API Tag
		{
			std::string Name = "Unnamed Entity";
		};

		struct HBL2_API ID
		{
			UUID Identifier = 0;
		};

		struct HBL2_API Transform
		{
			glm::vec3 Translation = { 0.f, 0.f, 0.f };
			glm::vec3 Rotation = { 0.f, 0.f, 0.f };
			glm::quat QRotation = glm::quat(Rotation);
			glm::vec3 Scale = { 1.f, 1.f, 1.f };

			glm::mat4 LocalMatrix = glm::mat4(1.f);
			glm::mat4 WorldMatrix = glm::mat4(1.f);
			bool Static = false;
		};

		struct HBL2_API Link
		{
			UUID Parent = 0;
		};

		struct HBL2_API Sprite_New
		{
			Handle<Material> Material;
			bool Enabled = true;
		};

		struct HBL2_API StaticMesh_New
		{
			Handle<Mesh> Mesh;
			Handle<Material> Material;
			bool Enabled = true;
		};

		struct HBL2_API Camera
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

		struct HBL2_API EditorVisible
		{
			static inline bool Selected = false;
			static inline entt::entity SelectedEntity = entt::null;

			bool Enabled = true;
		};

		struct HBL2_API Light
		{
			enum class Type
			{
				Directional = 0,
				Point,
			};

			float Intensity = 1.0f;
			glm::vec3 Color = { 1.0f, 1.0f, 1.0f };
			float Attenuation = 10.0f;
			Type Type = Type::Directional;
			bool CastsShadows = false;
			bool Enabled = true;
		};

		struct HBL2_API SkyLight
		{
			float Intensity = 1.0f;
			bool Enabled = true;
		};

		struct HBL2_API SoundSource
		{
			Handle<Sound> Sound;
			bool Enabled = true;
		};
	}
}