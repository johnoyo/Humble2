#pragma once

#include "Base.h"

#include "Resources\Handle.h"
#include "Asset\Asset.h"

#include "Physics\Physics.h"
#include "Utilities\Bounds.h"
#include "Utilities\JobSystem.h"
#include "Utilities\Collections\StaticArray.h"

#include <entt.hpp>
#include <glm\gtx\hash.hpp>

namespace HBL2
{
	struct Mesh;
	struct Material;
	struct Sound;
	struct Texture;
	struct Buffer;

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
			enum class EType
			{
				Perspective = 0,
				Orthographic = 1,
			};

			float Exposure = 1.0f;
			float Gamma = 2.2f;
			float ZoomLevel = 300.f;
			float Fov = 30.f;
			float Near = 0.1f;
			float Far = 1000.f;
			float AspectRatio = 1.778f;
			EType Type = EType::Perspective;

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
			static inline Entity SelectedEntity = Entity::Null;

			bool Enabled = true;
		};

		struct HBL2_API Light
		{
			enum class EType
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

			float ConstantBias = 0.002f;
			float SlopeBias = 0.0f;
			float NormalOffsetScale = 0.0f;
			float FieldOfView = 60.0f;
			
			EType Type = EType::Directional;
			bool CastsShadows = false;
			bool Enabled = true;
		};

		struct HBL2_API SkyLight
		{
			Handle<Texture> CubeMap;
			Handle<Material> CubeMapMaterial;
			Handle<Texture> EquirectangularMap;
			bool Converted = false;
			bool Enabled = true;
		};

		struct HBL2_API AudioListener
		{
			bool Enabled;
		};

		struct HBL2_API AudioSource
		{
			enum AudioFlags : uint8_t
			{
				Looping = 1 << 0,
				Spatialised = 1 << 1,
			};

			enum class PlaybackState : uint8_t
			{
				Stopped,
				Playing,
				Paused,
				Trigger,
				Resume,
			};

			Handle<Sound> Sound;
			float Volume = 1.0f; // from 0 to 1
			float Pitch = 1.f; // from 0.5 to 2
			uint8_t Flags = 0;
			PlaybackState State = PlaybackState::Stopped;

			uint32_t ChannelIndex = UINT32_MAX; // For internall use only.
		};

		struct HBL2_API Rigidbody2D
		{
			Physics::BodyType Type = Physics::BodyType::Static;
			bool FixedRotation = false;
			Physics::ID BodyId = Physics::InvalidID;

			bool Dirty = false;
			bool Enabled = true;
		};

		struct HBL2_API BoxCollider2D
		{
			glm::vec2 Offset = { 0.0f, 0.0f };
			glm::vec2 Size = { 0.5f, 0.5f };

			float Density = 1.0f;
			float Friction = 0.5f;
			float Restitution = 0.0f;

			Physics::ID ShapeId = Physics::InvalidID;

			bool Trigger = false;
			bool Dirty = false;
			bool Enabled = true;
		};

		struct HBL2_API Rigidbody
		{
			enum class EMotionQuality
			{
				Discrete,
				Continuos,
			};

			Physics::ID BodyID = Physics::InvalidID;
			float GravityFactor = 1.0f;
			float Friction = 0.2f;
			float Restitution = 0.1f;
			Physics::BodyType Type = Physics::BodyType::Static;
			float LinearDamping = 0.05f;
			float AngularDamping = 0.05f;
			float Mass = 1.0f;
			EMotionQuality MotionQuality = EMotionQuality::Discrete;
			bool Trigger = false;
			bool Dirty = false;
			bool Enabled = true;
		};

		struct HBL2_API BoxCollider
		{
			glm::vec3 Size = { 1.0f, 1.0f, 1.0f };
			bool Enabled = true;
		};

		struct HBL2_API SphereCollider
		{
			float Radius = 1.0f;
			bool Enabled = true;
		};

		struct HBL2_API CapsuleCollider
		{
			float Height = 0.1f;
			float Radius = 1.0f;
			bool Enabled = true;
		};

		struct HBL2_API PrefabInstance
		{
			UUID Id = 0;
			uint32_t Version = 0;
		};

		struct HBL2_API Terrain
		{
			enum class ENormaliseMode
			{
				LOCAL = 0,
				GLOBAL,
			};

			int32_t ChunkSize = 241;
			float MaxViewDst = 450.f;
			int32_t ChunksVisibleInViewDst = 0;
			uint32_t InEditorPreviewLevelOfDetail = 0;

			ENormaliseMode NormaliseMode = ENormaliseMode::GLOBAL;
			uint64_t Seed = 5;

			float ViewerMoveThresholdForChunkUpdate = 15.f;
			float SqrViewerMoveThresholdForChunkUpdate = ViewerMoveThresholdForChunkUpdate * ViewerMoveThresholdForChunkUpdate;

			glm::vec3 OldViewerPosition{};

			float Scale = 1.f;
			float NoiseScale = 25.f;
			uint32_t Octaves = 5;
			float Persistance = 0.5f;
			float Lacunarity = 2.f;
			glm::vec2 Offset = { 50.f, 0.f };

			float HeightMultiplier = 1.f;

			std::unordered_map<glm::ivec2, Entity> ChunksCache;

			struct TerrainChunkData
			{
				glm::ivec2 ViewedCoord{};
				std::vector<float> NoiseMap;
			};

			struct TerrainChunkMeshData
			{
				Entity Chunk = Entity::Null;
				int32_t Lod = -1;
				float* VertexBuffer = nullptr;
				uint32_t VertexCount = 0;
				uint32_t* IndexBuffer = nullptr;
				uint32_t IndexCount = 0;
			};

			JobContext ChunkDataContext{};
			ThreadSafeDeque<TerrainChunkData> ChunkDataQueue;

			JobContext ChunkMeshDataContext{};
			ThreadSafeDeque<TerrainChunkMeshData> ChunkMeshDataQueue;

			struct LodInfo
			{
				int32_t Lod;
				float VisibleDstThreshold;
			};

			StaticArray<LodInfo, 3> DetailLevels;

			Handle<Material> Material;

			bool Regenerate = true;

			Terrain() = default;

			Terrain(const Terrain& other)
				: ChunkSize(other.ChunkSize),
				  MaxViewDst(other.MaxViewDst),
				  ChunksVisibleInViewDst(other.ChunksVisibleInViewDst),
				  InEditorPreviewLevelOfDetail(other.InEditorPreviewLevelOfDetail),
				  NormaliseMode(other.NormaliseMode),
				  Seed(other.Seed),
				  NoiseScale(other.NoiseScale),
				  Octaves(other.Octaves),
				  Persistance(other.Persistance),
				  Lacunarity(other.Lacunarity),
				  Offset(other.Offset),
				  HeightMultiplier(other.HeightMultiplier),
				  DetailLevels(other.DetailLevels),
				  Material(other.Material),
				  Regenerate(other.Regenerate)
			{
				// NOTE: Only copy the data and parameters of the chunk, the other will be calculated from scratch.
				JobSystem::Get().Wait(ChunkDataContext);
				JobSystem::Get().Wait(ChunkMeshDataContext);

				JobSystem::Get().Wait(other.ChunkDataContext);
				JobSystem::Get().Wait(other.ChunkMeshDataContext);
			}

			Terrain& operator=(const Terrain& other)
			{
				if (this == &other)
				{
					return *this;
				}

				ChunkSize = other.ChunkSize;
				MaxViewDst = other.MaxViewDst;
				ChunksVisibleInViewDst = other.ChunksVisibleInViewDst;
				InEditorPreviewLevelOfDetail = other.InEditorPreviewLevelOfDetail;

				NormaliseMode = other.NormaliseMode;
				Seed = other.Seed;

				NoiseScale = other.NoiseScale;
				Octaves = other.Octaves;
				Persistance = other.Persistance;
				Lacunarity = other.Lacunarity;
				Offset = other.Offset;

				HeightMultiplier = other.HeightMultiplier;

				DetailLevels = other.DetailLevels;

				Material = other.Material;
				Regenerate = other.Regenerate;

				// NOTE: Only copy the data and parameters of the chunk, the other will be calculated from scratch.
				JobSystem::Get().Wait(ChunkDataContext);
				JobSystem::Get().Wait(ChunkMeshDataContext);

				JobSystem::Get().Wait(other.ChunkDataContext);
				JobSystem::Get().Wait(other.ChunkMeshDataContext);

				return *this;
			}
		};

		struct HBL2_API TerrainChunk
		{
			struct LodMesh
			{
				int32_t Lod = 0;
				bool HasMesh = false;
				bool HasRequestedMesh = false;

				Handle<Buffer> VertexBuffer;
				Handle<Buffer> IndexBuffer;
				Handle<Mesh> Mesh;
			};

			std::vector<float> NoiseMap;
			Bounds ChunkBounds;
			bool Visible = false;
			bool VisibleLastUpdate = false;
			uint32_t LevelOfDetail = 0;
			int32_t PreviousLodIndex = -1;

			StaticArray<LodMesh, 6> LodMeshes;
		};

		struct HBL2_API AnimationCurve
		{
			struct KeyFrame
			{
				float Time = 0.0f;
				float Value = 0.0f;
				float InTan = 0.0f;
				float OutTan = 0.0f;
			};

			enum class CurvePreset
			{
				Linear,
				QuadraticEaseIn,
				QuadraticEaseOut,
				CubicEaseIn,
				CubicEaseOut,
				Custom
			};

			CurvePreset Preset = CurvePreset::Linear;
			CurvePreset PrevPreset = CurvePreset::Linear; // For internal use only.
			std::vector<KeyFrame> Keys;
			bool RecalculateTangents = false;
		};
	}
}