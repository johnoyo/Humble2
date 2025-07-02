#include "TerrainSystem.h"

namespace HBL2
{
	void TerrainSystem::OnCreate()
	{
		m_ResourceManager = ResourceManager::Instance;

		m_Context->Group<Component::Terrain>(Get<Component::StaticMesh>)
			.Each([&](Component::Terrain& terrain, Component::StaticMesh& staticMesh)
			{
				const auto& noiseMap = GenerateNoiseMap(
					terrain.mapWidth,
					terrain.mapHeight,
					terrain.Seed,
					terrain.scale,
					terrain.octaves,
					terrain.persistance,
					terrain.lacunarity,
					terrain.offset);

				GenerateTerrainMeshData(noiseMap, terrain.mapWidth, terrain.mapHeight, terrain.HeightMultiplier);

				staticMesh.Mesh = m_Mesh;
			});
	}

	void TerrainSystem::OnUpdate(float ts)
	{
		m_Context->Group<Component::Terrain>(Get<Component::StaticMesh>)
			.Each([&](Component::Terrain& terrain, Component::StaticMesh& staticMesh)
			{
				if (terrain.Regenerate)
				{
					const auto& noiseMap = GenerateNoiseMap(
						terrain.mapWidth,
						terrain.mapHeight,
						terrain.Seed,
						terrain.scale,
						terrain.octaves,
						terrain.persistance,
						terrain.lacunarity,
						terrain.offset);

					GenerateTerrainMeshData(noiseMap, terrain.mapWidth, terrain.mapHeight, terrain.HeightMultiplier);

					staticMesh.Mesh = m_Mesh;

					terrain.Regenerate = false;
				}
			});
	}

	void TerrainSystem::OnDestroy()
	{
		if (m_ResourceManager == nullptr)
		{
			return;
		}

		m_ResourceManager->DeleteBuffer(m_IndexBuffer);
		m_ResourceManager->DeleteBuffer(m_VertexBuffer);
		m_ResourceManager->DeleteMesh(m_Mesh);
	}

	std::vector<float> TerrainSystem::GenerateNoiseMap(uint32_t mapWidth, uint32_t mapHeight, uint64_t seed, float scale, uint32_t octaves, float persistance, float lacunarity, const glm::vec2& offset)
	{
		std::vector<float> noiseMap(mapWidth * mapHeight);

		std::vector<glm::vec2> octaveOffsets(octaves);

		Random::Seed(seed);

		for (int i = 0; i < octaves; i++)
		{
			float offsetX = Random::Float(-100000, 100000) + offset.x;
			float offsetY = Random::Float(-100000, 100000) + offset.y;
			octaveOffsets[i] = { offsetX, offsetY };
		}

		if (scale <= 0)
		{
			scale = 0.0001f;
		}

		float maxNoiseHeight = std::numeric_limits<float>::min();
		float minNoiseHeight = std::numeric_limits<float>::max();

		float halfWidth = mapWidth / 2.f;
		float halfHeight = mapHeight / 2.f;

		for (int y = 0; y < mapHeight; y++)
		{
			for (int x = 0; x < mapWidth; x++)
			{
				float amplitude = 1;
				float frequency = 1;
				float noiseHeight = 0;

				for (int i = 0; i < octaves; i++)
				{
					float sampleX = (x - halfWidth) / scale * frequency + octaveOffsets[i].x;
					float sampleY = (y - halfHeight) / scale * frequency + octaveOffsets[i].y;

					float perlinValue = Math::PerlinNoise(sampleX, sampleY) * 2 - 1;
					noiseHeight += perlinValue * amplitude;

					amplitude *= persistance;
					frequency *= lacunarity;
				}

				if (noiseHeight > maxNoiseHeight)
				{
					maxNoiseHeight = noiseHeight;
				}
				else if (noiseHeight < minNoiseHeight)
				{
					minNoiseHeight = noiseHeight;
				}

				noiseMap[x * mapWidth + y] = noiseHeight;
			}
		}

		// Normalise noise map values.
		for (int y = 0; y < mapHeight; y++)
		{
			for (int x = 0; x < mapWidth; x++)
			{
				noiseMap[x * mapWidth + y] = Math::InverseLerp(minNoiseHeight, maxNoiseHeight, noiseMap[x * mapWidth + y]);
			}
		}

		return noiseMap;
	}
	
	void TerrainSystem::GenerateTerrainMeshData(const Span<const float> heightMap, uint32_t width, uint32_t height, float heightMultiplier)
	{
		m_ResourceManager->DeleteBuffer(m_IndexBuffer);
		m_ResourceManager->DeleteBuffer(m_VertexBuffer);
		m_ResourceManager->DeleteMesh(m_Mesh);

		// Counts
		uint32_t vertexCount = width * height;
		uint32_t quadCount = (width - 1) * (height - 1);
		uint32_t indexCount = quadCount * 6;

		// Allocate GPU-side buffers
		float* vertexBuffer = new float[vertexCount * 8]; // 3 pos + 3 norm + 2 uv
		uint32_t* indexBuffer = new uint32_t[indexCount];

		// Temp storage for positions, uvs, and accumulating normals:
		std::vector<glm::vec3> positions(vertexCount);
		std::vector<glm::vec2> uvs(vertexCount);
		std::vector<glm::vec3> normals(vertexCount, glm::vec3(0.0f));

		// Compute positions & uvs; build index list
		uint32_t vi = 0;      // vertex index
		uint32_t ii = 0;      // index index
		float topLeftX = (width - 1) / -2.0f;
		float topLeftZ = (height - 1) / 2.0f;

		for (uint32_t y = 0; y < height; ++y)
		{
			for (uint32_t x = 0; x < width; ++x, ++vi)
			{
				// Position
				positions[vi] = glm::vec3(
					topLeftX + x,
					heightMap[y * width + x] * heightMultiplier,
					topLeftZ - y
				);

				// UV
				uvs[vi] = glm::vec2(x / float(width), y / float(height));

				// Build two triangles per quad (if not on last row/col)
				if (x < width - 1 && y < height - 1)
				{
					int a = vi;
					int b = vi + width + 1;
					int c = vi + width;
					int d = vi + 1;

					// tri 1: a, b, c
					indexBuffer[ii++] = a;
					indexBuffer[ii++] = b;
					indexBuffer[ii++] = c;
					// tri 2: b, a, d
					indexBuffer[ii++] = b;
					indexBuffer[ii++] = a;
					indexBuffer[ii++] = d;
				}
			}
		}

		// Compute normals by accumulating face normals
		for (int t = 0; t < indexCount; t += 3)
		{
			uint32_t i0 = indexBuffer[t + 0];
			uint32_t i1 = indexBuffer[t + 1];
			uint32_t i2 = indexBuffer[t + 2];

			glm::vec3& p0 = positions[i0];
			glm::vec3& p1 = positions[i1];
			glm::vec3& p2 = positions[i2];

			glm::vec3 faceNorm = glm::normalize(glm::cross(p1 - p0, p2 - p0));

			normals[i0] += faceNorm;
			normals[i1] += faceNorm;
			normals[i2] += faceNorm;
		}

		// Normalize all vertex normals
		for (int i = 0; i < vertexCount; ++i)
		{
			normals[i] = glm::normalize(normals[i]);
		}

		// Merge into one vertex buffer.
		for (int i = 0, dst = 0; i < vertexCount; ++i)
		{
			// position
			vertexBuffer[dst++] = positions[i].x;
			vertexBuffer[dst++] = positions[i].y;
			vertexBuffer[dst++] = positions[i].z;
			// normal
			vertexBuffer[dst++] = normals[i].x;
			vertexBuffer[dst++] = normals[i].y;
			vertexBuffer[dst++] = normals[i].z;
			// uv
			vertexBuffer[dst++] = uvs[i].x;
			vertexBuffer[dst++] = uvs[i].y;
		}

		// Create GPU buffers
		m_IndexBuffer = m_ResourceManager->CreateBuffer({
			.debugName = "terrain-index-buffer",
			.usage = BufferUsage::INDEX,
			.byteSize = (uint32_t)sizeof(uint32_t) * indexCount,
			.initialData = indexBuffer,
		});

		m_VertexBuffer = m_ResourceManager->CreateBuffer({
			.debugName = "terrain-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.byteSize = (uint32_t)sizeof(float) * vertexCount * 8,
			.initialData = vertexBuffer,
		});

		// Create mesh object
		m_Mesh = m_ResourceManager->CreateMesh({
			.debugName = "terrain-mesh",
			.meshes = {
				{
					.debugName = "terrain-part",
					.subMeshes = {
						{
							.indexCount = uint32_t(indexCount),
							.vertexOffset = 0,
							.vertexCount = uint32_t(vertexCount),
						}
					},
					.indexBuffer = m_IndexBuffer,
					.vertexBuffers = { m_VertexBuffer },
				}
			}
		});

		delete[] vertexBuffer;
		delete[] indexBuffer;
	}
}