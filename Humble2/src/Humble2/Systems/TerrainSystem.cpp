#include "TerrainSystem.h"

#include "AnimationCurveSystem.h"
#include "Utilities\EntityPresets.h"
#include "Project\Project.h"

namespace HBL2
{
	static inline int32_t CeilDiv(int32_t a, int32_t b)
	{
		return (a + b - 1) / b;
	}

	static int32_t Gcd(int32_t a, int32_t b) { while (b) { int32_t t = a % b; a = b; b = t; } return a; }
	static int32_t Lcm(int32_t a, int32_t b) { return (a / Gcd(a, b)) * b; }

	static int32_t LodIncrement(int32_t lod) { return (lod == 0) ? 1 : lod * 2; }

	static float EvaluateFalloffValue(float value)
	{
		const float a = 3.f;
		const float b = 2.2f;

		return glm::pow(value, a) / (glm::pow(value, a) + glm::pow(b - b * value, a));
	}

	void TerrainSystem::OnCreate()
	{
		m_ResourceManager = ResourceManager::Instance;
		m_EditorScene = m_ResourceManager->GetScene(Context::EditorScene);

		m_Context->Group<Component::Terrain>(Get<Component::StaticMesh, Component::AnimationCurve>)
			.Each([this](Entity entity, Component::Terrain& terrain, Component::StaticMesh& staticMesh, Component::AnimationCurve& curve)
			{
				InitializeTerrain(terrain);

				m_Context->GetComponent<Component::Transform>(entity).Scale = { terrain.Scale, terrain.Scale, terrain.Scale };

				if (terrain.NormaliseMode == Component::Terrain::ENormaliseMode::LOCAL)
				{
					Asset* heightMapAsset = AssetManager::Instance->GetAssetMetadata(terrain.HeightMap);

					int32_t gridSize = (int32_t)std::sqrt((double)terrain.NumberOfChunks);

					int32_t totalQuads = terrain.Size - 1;
					int32_t chunkQuads = CeilDiv(totalQuads, gridSize);

					// Compute required alignment so every LOD hits the border vertex exactly.
					int32_t align = 1;
					for (uint32_t i = 0; i < terrain.DetailLevels.Size(); ++i)
					{
						align = Lcm(align, LodIncrement(terrain.DetailLevels[i].Lod));
					}

					// Snap up to that alignment.
					int32_t rem = chunkQuads % align;
					if (rem != 0)
					{
						chunkQuads += (align - rem);
					}

					// Force Size to match chunking perfectly (shared-edge rule).
					terrain.ChunkSize = chunkQuads; // quads per chunk (world stride)
					terrain.Size = gridSize * chunkQuads + 1; // vertices/samples per side

					terrain.FalloffMap = GenerateFallofMap(terrain, terrain.Size);

					std::vector<float> noiseMap;
					if (heightMapAsset == nullptr)
					{
						terrain.FixedNoiseMap = GenerateNoiseMap(terrain, terrain.Size, glm::vec2(0.f));
					}
					else
					{
						int w, h;
						terrain.FixedNoiseMap = LoadHeightmap(Project::GetAssetFileSystemPath(heightMapAsset->FilePath).string(), w, h, true); // TODO: Fix heightmap loading.
						terrain.Size = h;
					}
				}

				// Clear the serialized chunks.
				if (m_Context->HasComponent<Component::Link>(entity))
				{
					m_Context->GetComponent<Component::Link>(entity).Children.clear();
				}

				terrain.Initialized = true;
			});

		Scene* scene = (Context::Mode == Mode::Editor ? m_EditorScene : m_Context);
		const Component::Transform& viewer = scene->GetComponent<Component::Transform>(GetMainCamera());

		m_Context->Group<Component::Terrain>(Get<Component::StaticMesh, Component::AnimationCurve>)
			.Each([this, &viewer](Entity e, Component::Terrain& terrain, Component::StaticMesh& staticMesh, Component::AnimationCurve& curve)
			{
				UpdateVisibleChunks(e, terrain, curve, viewer);
			});
	}

	void TerrainSystem::OnUpdate(float ts)
	{
		BEGIN_PROFILE_SYSTEM();

		Scene* scene = (Context::Mode == Mode::Editor ? m_EditorScene : m_Context);

		const Component::Transform& viewer = scene->GetComponent<Component::Transform>(GetMainCamera());

		m_Context->Group<Component::Terrain>(Get<Component::StaticMesh, Component::AnimationCurve>)
			.Each([this, &viewer](Entity e, Component::Terrain& terrain, Component::StaticMesh& staticMesh, Component::AnimationCurve& curve)
			{
				if (!terrain.Initialized)
				{
					InitializeTerrain(terrain);
					terrain.Initialized = true;
				}

				if (terrain.NoiseScale <= 0)
				{
					terrain.NoiseScale = 0.0001f;
				}

				terrain.MaxViewDst = terrain.DetailLevels[terrain.DetailLevels.Size() - 1].VisibleDstThreshold;
				terrain.ChunksVisibleInViewDst = std::max(1, (int32_t)(terrain.MaxViewDst / terrain.ChunkSize));
				m_Context->GetComponent<Component::Transform>(e).Scale = { terrain.Scale, terrain.Scale, terrain.Scale };

				// Process chunck data (from job #JOB1).
				Component::Terrain::TerrainChunkData chunkData;
				while (terrain.ChunkDataQueue.try_dequeue(chunkData))
				{
					UUID terrainUUID = m_Context->GetComponent<Component::ID>(e).Identifier;

					int32_t size = INT32_MAX;
					switch (terrain.NormaliseMode)
					{
					case Component::Terrain::ENormaliseMode::LOCAL:
						terrain.ChunksCache[chunkData.ViewedCoord] = CreateEmptyChunk(terrain, chunkData, terrain.ChunkSize, terrainUUID);
						break;
					case Component::Terrain::ENormaliseMode::GLOBAL:
						terrain.ChunksCache[chunkData.ViewedCoord] = CreateEmptyChunk(terrain, chunkData, terrain.ChunkSize - 1, terrainUUID);
						break;
					}

					Entity chunk = terrain.ChunksCache[chunkData.ViewedCoord];
					UpdateTerrainChunk(chunk, terrain, curve, viewer);
				}

				// Process chunk mesh data (from job #JOB2).
				Component::Terrain::TerrainChunkMeshData chunkMeshData;
				while (terrain.ChunkMeshDataQueue.try_dequeue(chunkMeshData))
				{
					AssignChunkMesh(chunkMeshData);
					UpdateTerrainChunk(chunkMeshData.Chunk, terrain, curve, viewer);
				}

				if (terrain.Regenerate)
				{
					if (terrain.NormaliseMode == Component::Terrain::ENormaliseMode::LOCAL)
					{
						Asset* heightMapAsset = AssetManager::Instance->GetAssetMetadata(terrain.HeightMap);

						int32_t gridSize = (int32_t)std::sqrt((double)terrain.NumberOfChunks);

						int32_t totalQuads = terrain.Size - 1;
						int32_t chunkQuads = CeilDiv(totalQuads, gridSize);

						// Compute required alignment so every LOD hits the border vertex exactly.
						int32_t align = 1;
						for (uint32_t i = 0; i < terrain.DetailLevels.Size(); ++i)
						{
							align = Lcm(align, LodIncrement(terrain.DetailLevels[i].Lod));
						}

						// Snap up to that alignment.
						int32_t rem = chunkQuads % align;
						if (rem != 0)
						{
							chunkQuads += (align - rem);
						}

						// Force Size to match chunking perfectly (shared-edge rule).
						terrain.ChunkSize = chunkQuads; // quads per chunk (world stride)
						terrain.Size = gridSize * chunkQuads + 1; // vertices/samples per side

						terrain.FalloffMap = GenerateFallofMap(terrain, terrain.Size);

						if (heightMapAsset == nullptr)
						{
							terrain.FixedNoiseMap = GenerateNoiseMap(terrain, terrain.Size, glm::vec2(0.f));
						}
						else
						{
							int w, h;
							terrain.FixedNoiseMap = LoadHeightmap(Project::GetAssetFileSystemPath(heightMapAsset->FilePath).string(), w, h, true); // TODO: Fix heightmap loading.
							terrain.Size = h;
						}
					}

					CleanUpChunks();
					UpdateVisibleChunks(e, terrain, curve, viewer);

					terrain.Regenerate = false;

					return;
				}

				glm::vec3 scaledViewerPosition = viewer.Translation / terrain.Scale;

				glm::vec3 delta = scaledViewerPosition - terrain.OldViewerPosition;
				if (glm::dot(delta, delta) < terrain.SqrViewerMoveThresholdForChunkUpdate)
				{
					return;
				}

				terrain.OldViewerPosition = scaledViewerPosition;

				UpdateVisibleChunks(e, terrain, curve, viewer);
			});

		END_PROFILE_SYSTEM(RunningTime);
	}

	void TerrainSystem::OnDestroy()
	{
		if (m_ResourceManager == nullptr)
		{
			return;
		}

		CleanUpChunks();
	}

	void TerrainSystem::InitializeTerrain(Component::Terrain& terrain)
	{
		// Set the detail levels
		terrain.DetailLevels[0].Lod = 0;
		terrain.DetailLevels[0].VisibleDstThreshold = 200;

		terrain.DetailLevels[1].Lod = 2;
		terrain.DetailLevels[1].VisibleDstThreshold = 400;

		terrain.DetailLevels[2].Lod = 5;
		terrain.DetailLevels[2].VisibleDstThreshold = 600;

		// Calculate chunk visibility and max view distance
		terrain.MaxViewDst = terrain.DetailLevels[terrain.DetailLevels.Size() - 1].VisibleDstThreshold;
		terrain.ChunksVisibleInViewDst = std::max(1, (int32_t)(terrain.MaxViewDst / terrain.ChunkSize));

		terrain.Size = 724;
		terrain.NumberOfChunks = 9;
	}

	void TerrainSystem::UpdateFixedTerrainChunks(Entity terrainEntity, const Component::Transform& viewer, Component::Terrain& terrain, Component::AnimationCurve& curve)
	{
		glm::vec3 scaledViewerPosition = viewer.Translation / terrain.Scale;

		int32_t currentChunkCoordX = (int32_t)std::floor(scaledViewerPosition.x / terrain.ChunkSize);
		int32_t currentChunkCoordY = (int32_t)std::floor(scaledViewerPosition.z / terrain.ChunkSize);

		uint32_t numberOfChunks = terrain.NumberOfChunks;
		int32_t gridSize = (int32_t)std::sqrt((double)numberOfChunks);
		HBL2_CORE_ASSERT((uint32_t)(gridSize * gridSize) == numberOfChunks, "numberOfChunks must be a perfect square");

		// Center chunk coordinate is the terrain position.
		const glm::vec3& terrainTransform = m_Context->GetComponent<Component::Transform>(terrainEntity).Translation;
		glm::vec3 terrainLocalCenter = terrainTransform / terrain.Scale;
		glm::ivec2 centerCoord = { (int32_t)std::floor(terrainLocalCenter.x / terrain.ChunkSize), (int32_t)std::floor(terrainLocalCenter.z / terrain.ChunkSize) };

		// Compute fixed-terrain bounds in chunk coords.
		int32_t half = gridSize / 2;
		int32_t minCoordX = centerCoord.x - half;
		int32_t maxCoordX = centerCoord.x + ((gridSize % 2 == 0) ? (half - 1) : half);
		int32_t minCoordY = centerCoord.y - half;
		int32_t maxCoordY = centerCoord.y + ((gridSize % 2 == 0) ? (half - 1) : half);

		// Clamp the viewer-visible range to those bounds.
		int32_t minX = std::max(minCoordX, currentChunkCoordX - terrain.ChunksVisibleInViewDst);
		int32_t maxX = std::min(maxCoordX, currentChunkCoordX + terrain.ChunksVisibleInViewDst);
		int32_t minY = std::max(minCoordY, currentChunkCoordY - terrain.ChunksVisibleInViewDst);
		int32_t maxY = std::min(maxCoordY, currentChunkCoordY + terrain.ChunksVisibleInViewDst);

		// Iterate only within valid fixed-terrain bounds.
		for (int32_t y = minY; y <= maxY; ++y)
		{
			for (int32_t x = minX; x <= maxX; ++x)
			{
				glm::ivec2 viewedChunkCoord(x, y);

				if (terrain.ChunksCache.contains(viewedChunkCoord))
				{
					Entity chunk = terrain.ChunksCache[viewedChunkCoord];

					// If the chunk exists in the cache but the entity is Null,
					// it means that the job (#JOB1) that generates the chunk data has not finished yet, so skip.
					if (chunk == Entity::Null)
					{
						continue;
					}

					// Configure lod mesh or load if not yet loaded.
					UpdateTerrainChunk(chunk, terrain, curve, viewer);
				}
				else
				{
					terrain.ChunksCache[viewedChunkCoord] = Entity::Null;

					// Prepare chunk data (mainly get the chunk slice from the noise map). (#JOB1)
					JobSystem::Get().Execute(terrain.ChunkDataContext, [&terrain, &curve, viewedChunkCoord, this, x, y]()
					{
						HBL2_PROFILE("Create new chunk data")

						Component::Terrain::TerrainChunkData chunkData =
						{
							.ViewedCoord = viewedChunkCoord,
							.NoiseMap = GetChunkNoiseMap(terrain.FixedNoiseMap, terrain.Size, terrain.Size, terrain.ChunkSize, x, y),
						};

						terrain.ChunkDataQueue.enqueue(std::move(chunkData));
					});
				}
			}
		}
	}

	void TerrainSystem::UpdateInfiniteTerrainChunks(const Component::Transform& viewer, Component::Terrain& terrain, Component::AnimationCurve& curve)
	{
		glm::vec3 scaledViewerPosition = viewer.Translation / terrain.Scale;

		int32_t currentChunkCoordX = (int32_t)(scaledViewerPosition.x / (terrain.ChunkSize - 1));
		int32_t currentChunkCoordY = (int32_t)(scaledViewerPosition.z / (terrain.ChunkSize - 1));

		// Iterate over the visible chunks around the viewer position.
		for (int yOffset = -terrain.ChunksVisibleInViewDst; yOffset <= terrain.ChunksVisibleInViewDst; yOffset++)
		{
			for (int xOffset = -terrain.ChunksVisibleInViewDst; xOffset <= terrain.ChunksVisibleInViewDst; xOffset++)
			{
				// Get the chunk coord in [-1, 1] space around the viewer.
				glm::ivec2 viewedChunkCoord(currentChunkCoordX + xOffset, currentChunkCoordY + yOffset);

				if (terrain.ChunksCache.contains(viewedChunkCoord))
				{
					Entity chunk = terrain.ChunksCache[viewedChunkCoord];

					// If the chunk exists in the cache but the entity is Null,
					// it means that the job (#JOB1) that generates the chunk data has not finished yet, so skip.
					if (chunk == Entity::Null)
					{
						continue;
					}

					UpdateTerrainChunk(chunk, terrain, curve, viewer);
				}
				else
				{
					// Set placeholder entity for this chunk coord.
					terrain.ChunksCache[viewedChunkCoord] = Entity::Null;

					// Prepare chunk data (mainly generate the noise map). (#JOB1)
					JobSystem::Get().Execute(terrain.ChunkDataContext, [&terrain, &curve, viewedChunkCoord, this]()
					{
						HBL2_PROFILE("Create new chunk data")

						Component::Terrain::TerrainChunkData chunkData =
						{
							.ViewedCoord = viewedChunkCoord,
							.NoiseMap = GenerateNoiseMap(terrain, terrain.ChunkSize, viewedChunkCoord * (terrain.ChunkSize - 1)),
						};

						terrain.ChunkDataQueue.enqueue(std::move(chunkData));
					});
				}
			}
		}
	}

	std::vector<float> TerrainSystem::LoadHeightmap(const std::string& path, int& outWidth, int& outHeight, bool flipVertically, bool useRedChannelIfRGB)
	{
		stbi_set_flip_vertically_on_load(flipVertically);

		int w = 0;
		int h = 0;
		int channelsInFile = 0;
		std::vector<float> heights;

		// Detect bit depth (8-bit vs 16-bit per channel)
		const bool is16 = stbi_is_16_bit(path.c_str()) != 0;

		if (is16)
		{
			// 16-bit path
			uint16_t* pixels16 = nullptr;

			if (useRedChannelIfRGB)
			{
				// Keep native channels, then take R
				pixels16 = stbi_load_16(path.c_str(), &w, &h, &channelsInFile, /*req_comp*/ 0);
			}
			else
			{
				// Ask stb to convert to single-channel grayscale
				pixels16 = stbi_load_16(path.c_str(), &w, &h, &channelsInFile, /*req_comp*/ 1);
				// channelsInFile is still the original file channels, not req_comp.
			}

			if (!pixels16)
			{
				throw std::runtime_error(std::string("stbi_load_16 failed: ") + stbi_failure_reason());
			}

			outWidth = w;
			outHeight = h;
			const size_t count = static_cast<size_t>(w) * static_cast<size_t>(h);
			heights.resize(count);

			if (useRedChannelIfRGB)
			{
				if (channelsInFile <= 0)
				{
					stbi_image_free(pixels16);
					throw std::runtime_error("Invalid channel count in PNG.");
				}

				for (size_t i = 0; i < count; ++i)
				{
					const uint16_t r = pixels16[i * channelsInFile + 0];
					heights[i] = static_cast<float>(r) / 65535.0f;
				}
			}
			else
			{
				// Already 1-channel from req_comp = 1
				for (size_t i = 0; i < count; ++i)
				{
					heights[i] = static_cast<float>(pixels16[i]) / 65535.0f;
				}
			}

			stbi_image_free(pixels16);
		}
		else
		{
			// 8-bit path
			unsigned char* pixels8 = nullptr;

			if (useRedChannelIfRGB)
			{
				// Keep native channels, then take R
				pixels8 = stbi_load(path.c_str(), &w, &h, &channelsInFile, /*req_comp*/ 0);
			}
			else
			{
				// Ask stb to convert to single-channel grayscale
				pixels8 = stbi_load(path.c_str(), &w, &h, &channelsInFile, /*req_comp*/ 1);
			}

			if (!pixels8)
			{
				throw std::runtime_error(std::string("stbi_load failed: ") + stbi_failure_reason());
			}

			outWidth = w;
			outHeight = h;
			const size_t count = static_cast<size_t>(w) * static_cast<size_t>(h);
			heights.resize(count);

			if (useRedChannelIfRGB)
			{
				if (channelsInFile <= 0)
				{
					stbi_image_free(pixels8);
					throw std::runtime_error("Invalid channel count in PNG.");
				}
				for (size_t i = 0; i < count; ++i)
				{
					const unsigned char r = pixels8[i * channelsInFile + 0];
					heights[i] = static_cast<float>(r) / 255.0f;
				}
			}
			else
			{
				// Already 1-channel from req_comp = 1
				for (size_t i = 0; i < count; ++i)
				{
					heights[i] = static_cast<float>(pixels8[i]) / 255.0f;
				}
			}

			stbi_image_free(pixels8);
		}

		// Optional: reset flipping for future loads (global state).
		stbi_set_flip_vertically_on_load(false);
		return heights;
	}

	std::vector<float> TerrainSystem::GenerateFallofMap(const Component::Terrain& terrain, int32_t size)
	{
		std::vector<float> falloffMap(size * size);

		for (int i = 0; i < size; i++)
		{
			for (int j = 0; j < size; j++)
			{
				float x = i / (float)size * 2 - 1;
				float y = j / (float)size * 2 - 1;

				float value = glm::max(glm::abs(x), glm::abs(y));
				falloffMap[i + j * size] = EvaluateFalloffValue(value);
			}
		}

		return falloffMap;
	}

	std::vector<float> TerrainSystem::GenerateNoiseMap(const Component::Terrain& terrain, int32_t size, const glm::vec2& center)
	{
		std::vector<float> noiseMap(size * size);
		std::vector<glm::vec2> octaveOffsets(terrain.Octaves);

		float maxPossibleHeight = 0;
		float amplitude = 1;
		float frequency = 1;

		Random::Seed(terrain.Seed);

		for (int i = 0; i < terrain.Octaves; i++)
		{
			float offsetX = Random::Float(-100000, 100000) + terrain.Offset.x + center.x;
			float offsetY = Random::Float(-100000, 100000) - terrain.Offset.y - center.y;
			octaveOffsets[i] = { offsetX, offsetY };

			maxPossibleHeight += amplitude;
			amplitude *= terrain.Persistance;
		}

		float maxLocalNoiseHeight = std::numeric_limits<float>::min();
		float minLocalNoiseHeight = std::numeric_limits<float>::max();

		float halfWidth = size / 2.f;
		float halfHeight = size / 2.f;

		for (int y = 0; y < size; y++)
		{
			for (int x = 0; x < size; x++)
			{
				amplitude = 1;
				frequency = 1;
				float noiseHeight = 0;

				for (int i = 0; i < terrain.Octaves; i++)
				{
					float sampleX = (x - halfWidth + octaveOffsets[i].x) / terrain.NoiseScale * frequency;
					float sampleY = (y - halfHeight + octaveOffsets[i].y) / terrain.NoiseScale * frequency;

					float perlinValue = Math::PerlinNoise(sampleX, sampleY) * 2 - 1;
					noiseHeight += perlinValue * amplitude;

					amplitude *= terrain.Persistance;
					frequency *= terrain.Lacunarity;
				}

				if (noiseHeight > maxLocalNoiseHeight)
				{
					maxLocalNoiseHeight = noiseHeight;
				}
				else if (noiseHeight < minLocalNoiseHeight)
				{
					minLocalNoiseHeight = noiseHeight;
				}

				noiseMap[x + y * size] = noiseHeight;

				// Normalise noise map values.
				if (terrain.NormaliseMode == Component::Terrain::ENormaliseMode::GLOBAL)
				{
					float normalizedHeight = (noiseMap[x + y * size] + 1) / (maxPossibleHeight / 0.9f);
					noiseMap[x + y * size] = glm::clamp(normalizedHeight, 0.f, std::numeric_limits<float>::max());
				}
			}
		}

		// Normalise noise map values.
		if (terrain.NormaliseMode == Component::Terrain::ENormaliseMode::LOCAL)
		{
			for (int y = 0; y < size; y++)
			{
				for (int x = 0; x < size; x++)
				{
					float noiseValue = Math::InverseLerp(minLocalNoiseHeight, maxLocalNoiseHeight, noiseMap[x + y * size]);

					if (terrain.UseFalloffMap)
					{
						noiseMap[x + y * size] = glm::clamp(noiseValue - terrain.FalloffMap[x + y * size], 0.f, 1.f);
					}
					else
					{
						noiseMap[x + y * size] = noiseValue;
					}
				}
			}
		}

		return noiseMap;
	}
	
	std::vector<float> TerrainSystem::GetChunkNoiseMap(const std::vector<float>& src, int32_t srcW, int32_t srcH, int32_t chunkQuads, int32_t chunkX, int32_t chunkY)
	{
		if (srcW <= 0 || srcH <= 0 || chunkQuads <= 0)
		{
			return {};
		}

		const int64_t expectedSize = static_cast<int64_t>(srcW) * static_cast<int64_t>(srcH);
		if (expectedSize != static_cast<int64_t>(src.size()))
		{
			return {};
		}

		const int32_t chunkVerts = chunkQuads + 1;

		const int32_t chunksAcross = (srcW - 1) / chunkQuads;
		const int32_t chunksDown = (srcH - 1) / chunkQuads;

		const int32_t originX = chunksAcross / 2;
		const int32_t originY = chunksDown / 2;

		const int32_t idxX = chunkX + originX;
		const int32_t idxY = (-chunkY) + originY;

		if (idxX < 0 || idxX >= chunksAcross || idxY < 0 || idxY >= chunksDown)
		{
			return {};
		}

		const int32_t startX = idxX * chunkQuads;
		const int32_t startY = idxY * chunkQuads;

		if (startX + chunkVerts > srcW || startY + chunkVerts > srcH)
		{
			return {};
		}

		std::vector<float> out(static_cast<size_t>(chunkVerts) * static_cast<size_t>(chunkVerts));

		const float* srcPtr = src.data();
		float* dstPtr = out.data();
		const size_t copySizeBytes = static_cast<size_t>(chunkVerts) * sizeof(float);

		for (int32_t y = 0; y < chunkVerts; ++y)
		{
			const float* srcRow = srcPtr + static_cast<size_t>(startY + y) * srcW + startX;
			float* dstRow = dstPtr + static_cast<size_t>(y) * chunkVerts;
			std::memcpy(dstRow, srcRow, copySizeBytes);
		}

		return out;
	}

	void TerrainSystem::GenerateTerrainChunkMesh(const Span<const float> heightMap, uint32_t width, uint32_t height, float heightMultiplier, Component::AnimationCurve& curve, Component::Terrain::TerrainChunkMeshData& outChunkData)
	{
		HBL2_FUNC_PROFILE()

		uint32_t meshSimplificationIncrement = ((outChunkData.Lod == 0) ? 1 : outChunkData.Lod * 2);
		uint32_t verticesPerLine = ((width - 1) / meshSimplificationIncrement) + 1;

		// Counts
		uint32_t vertexCount = verticesPerLine * verticesPerLine;
		uint32_t quadCount = (verticesPerLine - 1) * (verticesPerLine - 1);
		uint32_t indexCount = quadCount * 6;

		// Allocate CPU-side buffers
		std::vector<float> vertexBuffer;
		vertexBuffer.resize(vertexCount * 8); // 3 pos + 3 norm + 2 uv

		std::vector<uint32_t> indexBuffer;
		indexBuffer.resize(indexCount);

		// Temp storage for positions, uvs, and accumulating normals:
		std::vector<glm::vec3> positions(vertexCount);
		std::vector<glm::vec2> uvs(vertexCount);
		std::vector<glm::vec3> normals(vertexCount, glm::vec3(0.0f));

		// Compute positions & uvs; build index list
		uint32_t vi = 0;      // vertex index
		uint32_t ii = 0;      // index index
		float topLeftX = (width - 1) / -2.0f;
		float topLeftZ = (height - 1) / 2.0f;

		for (uint32_t y = 0; y < height; y += meshSimplificationIncrement)
		{
			for (uint32_t x = 0; x < width; x += meshSimplificationIncrement, vi++)
			{
				// Position
				float heightY = AnimationCurveSystem::Evaluate(curve, heightMap[x + y * width]) * heightMultiplier;
				positions[vi] = glm::vec3(topLeftX + x, heightY, topLeftZ - y);

				// UV
				uvs[vi] = glm::vec2(x / float(width - 1), y / float(height - 1));

				// Build two triangles per quad (if not on last row/col)
				if (x + meshSimplificationIncrement < width && y + meshSimplificationIncrement < height)
				{
					uint32_t a = vi;
					uint32_t b = vi + verticesPerLine + 1;
					uint32_t c = vi + verticesPerLine;
					uint32_t d = vi + 1;

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
		for (uint32_t t = 0; t < indexCount; t += 3)
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

		Handle<Mesh> chunkMesh = m_ResourceManager->CreateMesh({
			.debugName = "terrain-mesh",
			.vertices = { vertexBuffer.data(), vertexBuffer.size()},
			.indeces = { indexBuffer.data(), indexBuffer.size() },
		});

		outChunkData.ChunkMeshHandle = chunkMesh;
	}

	void TerrainSystem::UpdateTerrainChunk(Entity chunk, Component::Terrain& terrain, Component::AnimationCurve& curve, const Component::Transform& viewer)
	{
		glm::vec3 scaledViewerPosition = viewer.Translation / terrain.Scale;

		auto& terrainChunk = m_Context->GetComponent<Component::TerrainChunk>(chunk);
		auto& chunkMesh = m_Context->GetComponent<Component::StaticMesh>(chunk);

		float viewerDstFromNearestEdge = glm::sqrt(terrainChunk.ChunkBounds.SqrDistance({ scaledViewerPosition.x, 0.0f, scaledViewerPosition.z }));

		bool visible = (viewerDstFromNearestEdge <= terrain.MaxViewDst);

		if (visible)
		{
			terrainChunk.VisibleLastUpdate = true;

			// Find which lod to use for the chunk.
			int32_t lodIndex = 0;

			for (uint32_t i = 0; i < terrain.DetailLevels.Size() - 1; i++)
			{
				if (viewerDstFromNearestEdge > terrain.DetailLevels[i].VisibleDstThreshold)
				{
					lodIndex = i + 1;
				}
				else
				{
					break;
				}
			}

			// If the lod that we have to use has changed update the chunk mesh.
			if (terrainChunk.PreviousLodIndex != lodIndex)
			{
				auto& lodMesh = terrainChunk.LodMeshes[terrain.DetailLevels[lodIndex].Lod];

				if (lodMesh.HasMesh)
				{
					terrainChunk.LevelOfDetail = lodMesh.Lod;
					terrainChunk.PreviousLodIndex = lodIndex;
					chunkMesh.Mesh = lodMesh.Mesh;
				}
				else if (!lodMesh.HasRequestedMesh)
				{
					// Prepare chunk mesh data (mainly generate the vertices, indices, normals). (#JOB2)
					JobSystem::Get().Execute(terrain.ChunkMeshDataContext, [&terrain, &terrainChunk, &curve, &lodMesh, chunk, this]()
					{
						HBL2_PROFILE("Create new chunk mesh data")

						Component::Terrain::TerrainChunkMeshData chunkMeshData =
						{
							.Chunk = chunk,
							.Lod = lodMesh.Lod,
						};

						int32_t size = INT32_MAX;
						switch (terrain.NormaliseMode)
						{
						case Component::Terrain::ENormaliseMode::LOCAL:
							// We need the +1 because N quads require N+1 vertices.
							// Adjacent chunks must share their border vertices,
							// so the global terrain size must be: totalSize = chunksPerSide * chunkQuads + 1
							// Without the +1, chunks would not share the edge vertex line,
							// which causes visible gaps between them.
							size = terrain.ChunkSize + 1;
							break;
						case Component::Terrain::ENormaliseMode::GLOBAL:
							size = terrain.ChunkSize;
							break;
						}

						GenerateTerrainChunkMesh(terrainChunk.NoiseMap, size, size, terrain.HeightMultiplier, curve, chunkMeshData);
						terrain.ChunkMeshDataQueue.enqueue(chunkMeshData);
					});

					lodMesh.HasRequestedMesh = true;
				}
			}
		}

		terrainChunk.Visible = visible;
		chunkMesh.Enabled = visible;
	}

	void TerrainSystem::UpdateVisibleChunks(Entity terrainEntity, Component::Terrain& terrain, Component::AnimationCurve& curve, const Component::Transform& viewer)
	{
		glm::vec3 scaledViewerPosition = viewer.Translation / terrain.Scale;

		float maxDistanceForChunkToStayLoaded = terrain.DetailLevels[terrain.DetailLevels.Size() - 1].VisibleDstThreshold * 2;

		ScratchArena scratch(Allocator::FrameArenaMT);
		DArray<Entity> chunks = MakeDArray<Entity>(scratch, 512);

		m_Context->Group<Component::TerrainChunk>(Get<Component::Transform, Component::StaticMesh>)
			.Each([&](Entity chunk, Component::TerrainChunk& terrainChunk, Component::Transform& tr, Component::StaticMesh& chunkMesh)
			{
				// Disable all the chunks visible last update.
				if (terrainChunk.VisibleLastUpdate)
				{
					chunkMesh.Enabled = false;
					terrainChunk.Visible = false;
					terrainChunk.VisibleLastUpdate = false;
				}
				
				float viewerDstFromNearestEdge = glm::sqrt(terrainChunk.ChunkBounds.SqrDistance( { scaledViewerPosition.x, 0.0f, scaledViewerPosition.z } ));

				if (viewerDstFromNearestEdge >= maxDistanceForChunkToStayLoaded)
				{
					// Clean up gpu buffers.
					for (auto& lodMesh : terrainChunk.LodMeshes)
					{
						// Skip chunk unload, since a job is generating a lod mesh atm.
						if (lodMesh.HasRequestedMesh && !lodMesh.HasMesh)
						{
							return;
						}

						Mesh* mesh = m_ResourceManager->GetMesh(lodMesh.Mesh);

						if (mesh == nullptr)
						{
							continue;
						}

						for (auto& meshPart : mesh->Meshes)
						{
							m_ResourceManager->DeleteBuffer(meshPart.IndexBuffer);

							for (auto& vertexBuffer : meshPart.VertexBuffers)
							{
								m_ResourceManager->DeleteBuffer(vertexBuffer);
							}
						}

						m_ResourceManager->DeleteMesh(lodMesh.Mesh);
					}

					// Store chunk entities to remove.
					chunks.push_back(chunk);

					// Remove the chunk entity from the terrain chunk cache.
					glm::ivec2 chunkCoordToRemove;
					bool chunkCoordToRemoveFound = false;

					for (const auto& [coord, entity] : terrain.ChunksCache)
					{
						if (entity == chunk)
						{
							chunkCoordToRemove = coord;
							chunkCoordToRemoveFound = true;
							break;
						}
					}

					if (chunkCoordToRemoveFound)
					{
						terrain.ChunksCache.erase(chunkCoordToRemove);
					}
				}
			});

		// Destroy chunk entities that need to be unloaded.
		for (auto chunk : chunks)
		{
			m_Context->DestroyEntity(chunk);
		}

		switch (terrain.NormaliseMode)
		{
			case Component::Terrain::ENormaliseMode::LOCAL:
				UpdateFixedTerrainChunks(terrainEntity, viewer, terrain, curve);
				break;
			case Component::Terrain::ENormaliseMode::GLOBAL:
				UpdateInfiniteTerrainChunks(viewer, terrain, curve);
				break;
		}
	}

	Entity TerrainSystem::CreateEmptyChunk(const Component::Terrain& terrain, Component::Terrain::TerrainChunkData& chunkData, int32_t chunkSize, UUID parent)
	{
		// Set a random seed again (TerrainSystem::GenerateNoiseMap had set a value).
		Random::Seed();

		Entity terrainChunk = m_Context->CreateEntity("TerrainChunk");

		auto& tr = m_Context->GetComponent<Component::Transform>(terrainChunk);
		tr.Translation = { (float)(chunkData.ViewedCoord.x * chunkSize), 0, (float)(chunkData.ViewedCoord.y * chunkSize) };
		tr.Scale = { 1.f, 1.f, 1.f };

		auto& meshComponent = m_Context->AddComponent<Component::StaticMesh>(terrainChunk);
		meshComponent.Enabled = false;
		meshComponent.Material = terrain.Material;
		meshComponent.Mesh = {};

		auto& link = m_Context->GetComponent<Component::Link>(terrainChunk);
		link.Parent = parent;

		auto& chunk = m_Context->AddComponent<Component::TerrainChunk>(terrainChunk);
		chunk.Visible = false;
		chunk.VisibleLastUpdate = false;
		chunk.ChunkBounds = Bounds({ chunkData.ViewedCoord.x * chunkSize, 0, chunkData.ViewedCoord.y * chunkSize }, glm::vec3(1.f, 0.0f, 1.0f) * (float)chunkSize);

		for (uint32_t i = 0; i < terrain.DetailLevels.Size(); i++)
		{
			chunk.LodMeshes[terrain.DetailLevels[i].Lod] = { terrain.DetailLevels[i].Lod };
		}

		chunk.NoiseMap = std::move(chunkData.NoiseMap);

		if (terrain.AddColliders)
		{
			auto& chunkRigidbody = m_Context->AddComponent<Component::Rigidbody>(terrainChunk);
			chunkRigidbody.Type = Physics::BodyType::Static;

			auto& chunkCollider = m_Context->AddComponent<Component::TerrainCollider>(terrainChunk);
			chunkCollider.ViewedCoord = chunkData.ViewedCoord;
		}

		return terrainChunk;
	}

	void TerrainSystem::AssignChunkMesh(const Component::Terrain::TerrainChunkMeshData& chunkMeshData)
	{
		HBL2_FUNC_PROFILE()

		auto& chunkMeshComponent = m_Context->GetComponent<Component::StaticMesh>(chunkMeshData.Chunk);
		chunkMeshComponent.Mesh = chunkMeshData.ChunkMeshHandle;

		auto& chunkComponent = m_Context->GetComponent<Component::TerrainChunk>(chunkMeshData.Chunk);
		chunkComponent.LevelOfDetail = chunkMeshData.Lod;

		auto& lodMesh = chunkComponent.LodMeshes[chunkMeshData.Lod];
		lodMesh.Mesh = chunkMeshData.ChunkMeshHandle;

		lodMesh.HasMesh = true;
	}

	Entity TerrainSystem::GetMainCamera()
	{
		if (Context::Mode == Mode::Runtime)
		{
			if (m_Context->MainCamera != Entity::Null)
			{
				return m_Context->MainCamera;
			}

			HBL2_CORE_WARN("No main camera set for runtime context.");
		}
		else if (Context::Mode == Mode::Editor)
		{
			if (m_EditorScene->MainCamera != Entity::Null)
			{
				return m_EditorScene->MainCamera;
			}

			HBL2_CORE_WARN("No main camera set for editor context.");
		}

		return Entity::Null;
	}
	
	void TerrainSystem::CleanUpChunks()
	{
		// Clean up resources of terrain chunks.
		ScratchArena scratch(Allocator::FrameArenaMT);
		DArray<Entity> chunks = MakeDArray<Entity>(scratch, 512);

		m_Context->View<Component::TerrainChunk>()
			.Each([this, &chunks](Entity chunk, Component::TerrainChunk& terrainChunk)
			{
				for (auto& lodMesh : terrainChunk.LodMeshes)
				{
					Mesh* mesh = m_ResourceManager->GetMesh(lodMesh.Mesh);

					if (mesh == nullptr)
					{
						continue;
					}

					for (auto& meshPart : mesh->Meshes)
					{
						m_ResourceManager->DeleteBuffer(meshPart.IndexBuffer);

						for (auto& vertexBuffer : meshPart.VertexBuffers)
						{
							m_ResourceManager->DeleteBuffer(vertexBuffer);
						}
					}

					m_ResourceManager->DeleteMesh(lodMesh.Mesh);
				}

				chunks.push_back(chunk);
			});

		// NOTE: We need to do such excessive clean up here because of play mode logic.
		//		 When we enter play mode, we clone the scene, unload it (call OnDestroy) and load the cloned one.
		//		 When exiting play mode, we unload and destroy the cloned scene and the we load the initial one.
		//		 We want to preserve any unsaved changes made to that scene before entering play mode, so we dont completely delete and reload from disk.
		//		 So we have to carefully clean up the scene, so when it gets loaded again after exiting play mode, it has a valid state.
		//		 In this system, its more complicated since we have to reinstantiate the terrain chunks each time from scratch, since they are not serialized.

		// Destroy chunk entities.
		for (auto chunk : chunks)
		{
			m_Context->DestroyEntity(chunk);
		}

		// Clear the chunk cache stored in terrains.
		m_Context->View<Component::Terrain>()
			.Each([](Component::Terrain& terrain)
			{
				terrain.ChunksCache.clear();

				JobSystem::Get().Wait(terrain.ChunkDataContext);

				Component::Terrain::TerrainChunkData chunkData;
				while (terrain.ChunkDataQueue.try_dequeue(chunkData))
				{
					// noop
				}

				JobSystem::Get().Wait(terrain.ChunkMeshDataContext);

				Component::Terrain::TerrainChunkMeshData chunkMeshData;
				while (terrain.ChunkMeshDataQueue.try_dequeue(chunkMeshData))
				{
					// noop
				}
			});
	}
}