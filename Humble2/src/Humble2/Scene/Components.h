#pragma once

#include "Base.h"

#include "Resources\Handle.h"
#include "Asset\Asset.h"

#include <entt.hpp>
#include <box2d\id.h>

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
			UUID PrevParent = 0;
			std::vector<UUID> Children;
		};

		struct HBL2_API Sprite
		{
			Handle<Material> Material;
			bool Enabled = true;
		};

		struct HBL2_API StaticMesh
		{
			Handle<Mesh> Mesh;
			uint32_t MeshIndex = 0;
			uint32_t SubMeshIndex = 0;
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

			struct FrustumPlane
			{
				glm::vec3 normal{};
				float distance = 0.0f;
			};

			struct CameraFrustum
			{
				FrustumPlane Planes[6]{};
			};
			
			CameraFrustum Frustum{};

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
				Spot,
			};

			float Intensity = 1.0f;
			glm::vec3 Color = { 1.0f, 1.0f, 1.0f };

			float InnerCutOff = 12.5f;
			float OuterCutOff = 17.5f;
			float Distance = 50.0f;
			
			Type Type = Type::Directional;
			bool CastsShadows = false;
			bool Enabled = true;
		};

		struct HBL2_API SkyLight
		{
			float Intensity = 1.0f;
			bool Enabled = true;
		};

		struct HBL2_API AudioSource
		{
			Handle<Sound> Sound;
			bool Enabled = true;
		};

		struct HBL2_API Rigidbody2D
		{
			enum class BodyType
			{
				Static = 0,
				Dynamic,
				Kinematic,
			};

			BodyType Type = BodyType::Static;
			bool FixedRotation = false;
			b2BodyId BodyId;

			bool Enabled = true;
		};

		struct HBL2_API BoxCollider2D
		{
			glm::vec2 Offset = { 0.0f, 0.0f };
			glm::vec2 Size = { 0.5f, 0.5f };

			float Density = 1.0f;
			float Friction = 0.5f;
			float Restitution = 0.0f;

			b2ShapeId ShapeId;

			bool Enabled = true;
		};
	}
}