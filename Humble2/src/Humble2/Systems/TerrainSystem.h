#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"

#include "Resources\ResourceManager.h"

#include <glm\glm.hpp>
#include <glm\gtx\quaternion.hpp>
#include <glm\gtx\hash.hpp>

namespace HBL2
{
	class HBL2_API TerrainSystem final : public ISystem
	{
	public:
		TerrainSystem() { Name = "TerrainSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnDestroy() override;

	private:
		void InitializeTerrain(Component::Terrain& terrain);
		void UpdateFixedTerrainChunks(const Component::Transform& viewer, Component::Terrain& terrain, Component::AnimationCurve& curve);
		void UpdateInfiniteTerrainChunks(const Component::Transform& viewer, Component::Terrain& terrain, Component::AnimationCurve& curve);

		std::vector<float> LoadHeightmap(const std::string& path, int& outWidth, int& outHeight, bool flipVertically = false, bool useRedChannelIfRGB = false);
		std::vector<float> GenerateNoiseMap(const Component::Terrain& terrain, int32_t size, const glm::vec2& center);
		void GenerateTerrainMeshData(const Span<const float> heightMap, uint32_t width, uint32_t height, float heightMultiplier, Component::AnimationCurve& curve, uint32_t levelOfDetail);
		void GenerateTerrainChunkMesh(const Span<const float> heightMap, uint32_t width, uint32_t height, float heightMultiplier, Component::AnimationCurve& curve, Component::Terrain::TerrainChunkMeshData& outChunkData);

		void UpdateTerrainChunk(Entity chunk, Component::Terrain& terrain, Component::AnimationCurve& curve, const Component::Transform& viewer);
		void UpdateVisibleChunks(Component::Terrain& terrain, Component::AnimationCurve& curve, const Component::Transform& viewer);

		Entity CreateEmptyChunk(const Component::Terrain& terrain, Component::Terrain::TerrainChunkData& chunkData, int32_t chunkSize, UUID parent);
		void AssignChunkMesh(const Component::Terrain::TerrainChunkMeshData& chunkMeshData);
		Entity GetMainCamera();

		void CleanUpChunks();

	private:
		ResourceManager* m_ResourceManager = nullptr;
		Scene* m_EditorScene = nullptr;

		Handle<Mesh> m_Mesh;
		Handle<Buffer> m_VertexBuffer;
		Handle<Buffer> m_IndexBuffer;
	};
}