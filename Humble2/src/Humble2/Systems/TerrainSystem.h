#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"

#include "Resources\ResourceManager.h"

#include <glm\glm.hpp>
#include <glm\gtx\quaternion.hpp>

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
		std::vector<float> GenerateNoiseMap(uint32_t mapWidth, uint32_t mapHeight, uint64_t seed, float scale, uint32_t octaves, float persistance, float lacunarity, const glm::vec2& offset);
		void GenerateTerrainMeshData(const Span<const float> heightMap, uint32_t width, uint32_t height, float heightMultiplier);

	private:
		ResourceManager* m_ResourceManager = nullptr;

		Handle<Mesh> m_Mesh;
		Handle<Buffer> m_VertexBuffer;
		Handle<Buffer> m_IndexBuffer;
	};
}