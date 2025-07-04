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
		std::vector<float> GenerateNoiseMap(Component::Terrain& terrain, const glm::vec2& center);
		void GenerateTerrainMeshData(const Span<const float> heightMap, uint32_t width, uint32_t height, float heightMultiplier, Component::AnimationCurve& curve, uint32_t levelOfDetail);
		Handle<Mesh> GenerateTerrainChunkMesh(const Span<const float> heightMap, uint32_t width, uint32_t height, float heightMultiplier, Component::AnimationCurve& curve, uint32_t levelOfDetail);

		Entity CreateChunk(const glm::ivec2& coord, int32_t chunkSize, UUID parent, Handle<Mesh> mesh, Handle<Material> material);
		Entity GetMainCamera();

	private:
		ResourceManager* m_ResourceManager = nullptr;
		Scene* m_EditorScene = nullptr;

		Handle<Mesh> m_Mesh;
		Handle<Buffer> m_VertexBuffer;
		Handle<Buffer> m_IndexBuffer;
	};
}