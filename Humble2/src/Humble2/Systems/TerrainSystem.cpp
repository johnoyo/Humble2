#include "TerrainSystem.h"

#include "AnimationCurveSystem.h"
#include "Utilities\EntityPresets.h"

namespace HBL2
{
	void TerrainSystem::OnCreate()
	{
		m_ResourceManager = ResourceManager::Instance;
		m_EditorScene = m_ResourceManager->GetScene(Context::EditorScene);

		m_Context->Group<Component::Terrain>(Get<Component::StaticMesh, Component::AnimationCurve>)
			.Each([&](Entity entity, Component::Terrain& terrain, Component::StaticMesh& staticMesh, Component::AnimationCurve& curve)
			{
				// Create and set up the preview mesh of terrain.
				const auto& noiseMap = GenerateNoiseMap(terrain, glm::vec2(0.f));
				GenerateTerrainMeshData(noiseMap, terrain.ChunkSize, terrain.ChunkSize, terrain.HeightMultiplier, curve, terrain.InEditorPreviewLevelOfDetail);
				staticMesh.Mesh = m_Mesh;

				// Set the detail levels
				terrain.DetailLevels[0].Lod = 0;
				terrain.DetailLevels[0].VisibleDstThreshold = 200;

				terrain.DetailLevels[1].Lod = 2;
				terrain.DetailLevels[1].VisibleDstThreshold = 400;

				terrain.DetailLevels[2].Lod = 5;
				terrain.DetailLevels[2].VisibleDstThreshold = 600;

				// Calculate chunk visibility and max view distance
				terrain.MaxViewDst = terrain.DetailLevels[terrain.DetailLevels.Size() - 1].VisibleDstThreshold;
				terrain.ChunksVisibleInViewDst = (int32_t)(terrain.MaxViewDst / terrain.ChunkSize);

				// Clear the serialized chunks.
				if (m_Context->HasComponent<Component::Link>(entity))
				{
					m_Context->GetComponent<Component::Link>(entity).Children.clear();
				}
			});

		Scene* scene = (Context::Mode == Mode::Editor ? m_EditorScene : m_Context);
		const Component::Transform& viewer = scene->GetComponent<Component::Transform>(GetMainCamera());

		m_Context->Group<Component::Terrain>(Get<Component::StaticMesh, Component::AnimationCurve>)
			.Each([&](Entity e, Component::Terrain& terrain, Component::StaticMesh& staticMesh, Component::AnimationCurve& curve)
			{
				UpdateVisibleChunks(terrain, curve, viewer);
			});
	}

	void TerrainSystem::OnUpdate(float ts)
	{
		Scene* scene = (Context::Mode == Mode::Editor ? m_EditorScene : m_Context);

		const Component::Transform& viewer = scene->GetComponent<Component::Transform>(GetMainCamera());

		m_Context->Group<Component::Terrain>(Get<Component::StaticMesh, Component::AnimationCurve>)
			.Each([&](Entity e, Component::Terrain& terrain, Component::StaticMesh& staticMesh, Component::AnimationCurve& curve)
			{
				if (terrain.NoiseScale <= 0)
				{
					terrain.NoiseScale = 0.0001f;
				}

				if (terrain.NormaliseMode == Component::Terrain::ENormaliseMode::LOCAL)
				{
					if (terrain.Regenerate)
					{
						const auto& noiseMap = GenerateNoiseMap(terrain, glm::vec2(0.f));
						GenerateTerrainMeshData(noiseMap, terrain.ChunkSize, terrain.ChunkSize, terrain.HeightMultiplier, curve, terrain.InEditorPreviewLevelOfDetail);
						staticMesh.Mesh = m_Mesh;

						terrain.MaxViewDst = terrain.DetailLevels[terrain.DetailLevels.Size() - 1].VisibleDstThreshold;
						terrain.ChunksVisibleInViewDst = (int32_t)(terrain.MaxViewDst / terrain.ChunkSize);

						terrain.Regenerate = false;
					}

					return;
				}

				// Process chunck data (from job #JOB1).
				while (!terrain.ChunkDataQueue.IsEmpty())
				{
					Component::Terrain::TerrainChunkData chunkData;
					if (terrain.ChunkDataQueue.PopFront(chunkData))
					{
						UUID terrainUUID = m_Context->GetComponent<Component::ID>(e).Identifier;
						terrain.ChunksCache[chunkData.ViewedCoord] = CreateEmptyChunk(terrain, chunkData, terrain.ChunkSize - 1, terrainUUID);

						Entity chunk = terrain.ChunksCache[chunkData.ViewedCoord];
						UpdateTerrainChunk(chunk, terrain, curve, viewer);
					}
				}

				// Process chunk mesh data (from job #JOB2).
				while (!terrain.ChunkMeshDataQueue.IsEmpty())
				{
					Component::Terrain::TerrainChunkMeshData chunkMeshData;
					if (terrain.ChunkMeshDataQueue.PopFront(chunkMeshData))
					{
						CreateChunkMesh(chunkMeshData);
						UpdateTerrainChunk(chunkMeshData.Chunk, terrain, curve, viewer);
					}
				}

				if (terrain.Regenerate)
				{
					CleanUpChunks();
					UpdateVisibleChunks(terrain, curve, viewer);

					terrain.Regenerate = false;

					return;
				}

				glm::vec3 scaledViewerPosition = viewer.Translation / terrain.Scale;

				if (glm::dot(terrain.OldViewerPosition, scaledViewerPosition) < terrain.SqrViewerMoveThresholdForChunkUpdate)
				{
					terrain.OldViewerPosition = scaledViewerPosition;
					return;
				}

				UpdateVisibleChunks(terrain, curve, viewer);
			});
	}

	void TerrainSystem::OnDestroy()
	{
		if (m_ResourceManager == nullptr)
		{
			return;
		}

		// Clean up resources of preview terrain.
		m_ResourceManager->DeleteBuffer(m_IndexBuffer);
		m_ResourceManager->DeleteBuffer(m_VertexBuffer);
		m_ResourceManager->DeleteMesh(m_Mesh);

		CleanUpChunks();
	}

	std::vector<float> TerrainSystem::GenerateNoiseMap(const Component::Terrain& terrain, const glm::vec2& center)
	{
		std::vector<float> noiseMap(terrain.ChunkSize * terrain.ChunkSize);

		std::vector<glm::vec2> octaveOffsets(terrain.Octaves);

		float maxPossibleHeight = 0;
		float amplitude = 1;
		float frequency = 1;

		Random::Seed(terrain.Seed);

		for (int i = 0; i < terrain.Octaves; i++)
		{
			float offsetX = Random::Float(-100000, 100000) - (terrain.Offset.x + center.y);
			float offsetY = Random::Float(-100000, 100000) + (terrain.Offset.y + center.x);
			octaveOffsets[i] = { offsetX, offsetY };

			maxPossibleHeight += amplitude;
			amplitude *= terrain.Persistance;
		}

		float maxLocalNoiseHeight = std::numeric_limits<float>::min();
		float minLocalNoiseHeight = std::numeric_limits<float>::max();

		float halfWidth = terrain.ChunkSize / 2.f;
		float halfHeight = terrain.ChunkSize / 2.f;

		for (int y = 0; y < terrain.ChunkSize; y++)
		{
			for (int x = 0; x < terrain.ChunkSize; x++)
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

				noiseMap[x * terrain.ChunkSize + y] = noiseHeight;
			}
		}

		// Normalise noise map values.
		for (int y = 0; y < terrain.ChunkSize; y++)
		{
			for (int x = 0; x < terrain.ChunkSize; x++)
			{
				if (terrain.NormaliseMode == Component::Terrain::ENormaliseMode::LOCAL)
				{
					noiseMap[x * terrain.ChunkSize + y] = Math::InverseLerp(minLocalNoiseHeight, maxLocalNoiseHeight, noiseMap[x * terrain.ChunkSize + y]);
				}
				else
				{
					float normalizedHeight = (noiseMap[x * terrain.ChunkSize + y] + 1) / (maxPossibleHeight / 0.9f);
					noiseMap[x * terrain.ChunkSize + y] = glm::clamp(normalizedHeight, 0.f, std::numeric_limits<float>::max());
				}
			}
		}

		return noiseMap;
	}
	
	void TerrainSystem::GenerateTerrainMeshData(const Span<const float> heightMap, uint32_t width, uint32_t height, float heightMultiplier, Component::AnimationCurve& curve, uint32_t levelOfDetail)
	{
		m_ResourceManager->DeleteBuffer(m_IndexBuffer);
		m_ResourceManager->DeleteBuffer(m_VertexBuffer);
		m_ResourceManager->DeleteMesh(m_Mesh);

		uint32_t meshSimplificationIncrement = ((levelOfDetail == 0) ? 1 : levelOfDetail * 2);
		uint32_t verticesPerLine = ((width - 1) / meshSimplificationIncrement) + 1;

		// Counts
		uint32_t vertexCount = verticesPerLine * verticesPerLine;
		uint32_t quadCount = (verticesPerLine - 1) * (verticesPerLine - 1);
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

		for (uint32_t y = 0; y < height; y += meshSimplificationIncrement)
		{
			for (uint32_t x = 0; x < width; x += meshSimplificationIncrement, vi++)
			{
				// Position
				positions[vi] = glm::vec3(
					topLeftX + x,
					AnimationCurveSystem::Evaluate(curve, heightMap[y * width + x]) * heightMultiplier,
					topLeftZ - y
				);

				// UV
				uvs[vi] = glm::vec2(x / float(width), y / float(height));

				// Build two triangles per quad (if not on last row/col)
				if (x < width - 1 && y < height - 1)
				{
					int a = vi;
					int b = vi + verticesPerLine + 1;
					int c = vi + verticesPerLine;
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

	void TerrainSystem::GenerateTerrainChunkMesh(const Span<const float> heightMap, uint32_t width, uint32_t height, float heightMultiplier, Component::AnimationCurve& curve, Component::Terrain::TerrainChunkMeshData& outChunkData)
	{
		HBL2_FUNC_PROFILE()

		uint32_t meshSimplificationIncrement = ((outChunkData.Lod == 0) ? 1 : outChunkData.Lod * 2);
		uint32_t verticesPerLine = ((width - 1) / meshSimplificationIncrement) + 1;

		// Counts
		uint32_t vertexCount = verticesPerLine * verticesPerLine;
		uint32_t quadCount = (verticesPerLine - 1) * (verticesPerLine - 1);
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

		for (uint32_t y = 0; y < height; y += meshSimplificationIncrement)
		{
			for (uint32_t x = 0; x < width; x += meshSimplificationIncrement, vi++)
			{
				// Position
				positions[vi] = glm::vec3(
					topLeftX + x,
					AnimationCurveSystem::Evaluate(curve, heightMap[y * width + x]) * heightMultiplier,
					topLeftZ - y
				);

				// UV
				uvs[vi] = glm::vec2(x / float(width), y / float(height));

				// Build two triangles per quad (if not on last row/col)
				if (x < width - 1 && y < height - 1)
				{
					int a = vi;
					int b = vi + verticesPerLine + 1;
					int c = vi + verticesPerLine;
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

		outChunkData.VertexBuffer = vertexBuffer;
		outChunkData.VertexCount = vertexCount;
		outChunkData.IndexBuffer = indexBuffer;
		outChunkData.IndexCount = indexCount;
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

						GenerateTerrainChunkMesh(terrainChunk.NoiseMap, terrain.ChunkSize, terrain.ChunkSize, terrain.HeightMultiplier, curve, chunkMeshData);
						terrain.ChunkMeshDataQueue.PushBack(chunkMeshData);
					});

					lodMesh.HasRequestedMesh = true;
				}
			}
		}

		terrainChunk.Visible = visible;
		chunkMesh.Enabled = visible;
	}

	void TerrainSystem::UpdateVisibleChunks(Component::Terrain& terrain, Component::AnimationCurve& curve, const Component::Transform& viewer)
	{
		m_Context->Group<Component::TerrainChunk>(Get<Component::StaticMesh>)
			.Each([&](Component::TerrainChunk& terrainChunk, Component::StaticMesh& chunkMesh)
			{
				if (terrainChunk.VisibleLastUpdate)
				{
					chunkMesh.Enabled = false;
					terrainChunk.Visible = false;
					terrainChunk.VisibleLastUpdate = false;
				}
			});

		glm::vec3 scaledViewerPosition = viewer.Translation / terrain.Scale;

		int32_t currentChunkCoordX = (int32_t)(scaledViewerPosition.x / terrain.ChunkSize);
		int32_t currentChunkCoordY = (int32_t)(scaledViewerPosition.z / terrain.ChunkSize);

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
						};

						chunkData.NoiseMap = GenerateNoiseMap(terrain, viewedChunkCoord * (terrain.ChunkSize - 1));

						terrain.ChunkDataQueue.PushBack(chunkData);
					});
				}
			}
		}
	}

	Entity TerrainSystem::CreateEmptyChunk(const Component::Terrain& terrain, Component::Terrain::TerrainChunkData& chunkData, int32_t chunkSize, UUID parent)
	{
		// Set a random seed again (TerrainSystem::GenerateNoiseMap had set a value).
		Random::Seed();

		Entity terrainChunk = m_Context->CreateEntity("TerrainChunk");

		auto& tr = m_Context->GetComponent<Component::Transform>(terrainChunk);
		tr.Translation = { (float)(chunkData.ViewedCoord.x * chunkSize) * terrain.Scale, 0, (float)(chunkData.ViewedCoord.y * chunkSize) * terrain.Scale };
		tr.Scale = { terrain.Scale, terrain.Scale, terrain.Scale };

		auto& meshComponent = m_Context->AddComponent<Component::StaticMesh>(terrainChunk);
		meshComponent.Enabled = false;
		meshComponent.Material = terrain.Material;
		meshComponent.Mesh = {};

		auto& link = m_Context->AddComponent<Component::Link>(terrainChunk);
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

		return terrainChunk;
	}

	void TerrainSystem::CreateChunkMesh(Component::Terrain::TerrainChunkMeshData& chunkMeshData)
	{
		HBL2_FUNC_PROFILE()

		// Create GPU buffers
		Handle<Buffer> indexBufferHandle = m_ResourceManager->CreateBuffer({
			.debugName = "terrain-index-buffer",
			.usage = BufferUsage::INDEX,
			.byteSize = (uint32_t)sizeof(uint32_t) * chunkMeshData.IndexCount,
			.initialData = chunkMeshData.IndexBuffer,
		});

		Handle<Buffer> vertexBufferHandle = m_ResourceManager->CreateBuffer({
			.debugName = "terrain-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.byteSize = (uint32_t)sizeof(float) * chunkMeshData.VertexCount * 8,
			.initialData = chunkMeshData.VertexBuffer,
		});

		// Create mesh resource.
		Handle<Mesh> chunkMesh = m_ResourceManager->CreateMesh({
			.debugName = "terrain-mesh",
			.meshes = {
				{
					.debugName = "terrain-part",
					.subMeshes = {
						{
							.indexCount = chunkMeshData.IndexCount,
							.vertexOffset = 0,
							.vertexCount = chunkMeshData.VertexCount,
						}
					},
					.indexBuffer = indexBufferHandle,
					.vertexBuffers = { vertexBufferHandle },
				}
			}
		});

		// Free cpu side buffer data.
		delete[] chunkMeshData.VertexBuffer;
		delete[] chunkMeshData.IndexBuffer;

		auto& chunkMeshComponent = m_Context->GetComponent<Component::StaticMesh>(chunkMeshData.Chunk);
		chunkMeshComponent.Mesh = chunkMesh;

		auto& chunkComponent = m_Context->GetComponent<Component::TerrainChunk>(chunkMeshData.Chunk);
		chunkComponent.LevelOfDetail = chunkMeshData.Lod;

		auto& lodMesh = chunkComponent.LodMeshes[chunkMeshData.Lod];
		lodMesh.Mesh = chunkMesh;
		lodMesh.IndexBuffer = indexBufferHandle;
		lodMesh.VertexBuffer = vertexBufferHandle;

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
		std::vector<Entity> chunks;

		m_Context->View<Component::TerrainChunk>()
			.Each([&](Entity chunk, Component::TerrainChunk& terrainChunk)
			{
				for (auto& lodMesh : terrainChunk.LodMeshes)
				{
					m_ResourceManager->DeleteBuffer(lodMesh.IndexBuffer);
					m_ResourceManager->DeleteBuffer(lodMesh.VertexBuffer);
					m_ResourceManager->DeleteMesh(lodMesh.Mesh);
				}

				chunks.push_back(chunk);
			});

		// NOTE: We need to do such excessive clean up here because of play mode logic.
		//		 When we enter play mode, we clone the scene, unload it (call OnDestroy) and load the cloned one.
		//		 When exiting play mode, we unload and destroy the cloned scene and the we load the initial one.
		//		 We want to preserve any unsaved changes made to that scene before entering play mode, so we dont completely delete and reload from disk.
		//		 So we have to carefully clean up the scene, so when it gets loaded again after exiting play mode, it has a valid state.
		//		 In this system its more complicated since we have to reinstantiate the terrain chunks each time from scratch, they are not serialized.

		// Destroy chunk entities.
		for (auto chunk : chunks)
		{
			m_Context->DestroyEntity(chunk);
		}

		// Clear the chunk cache stored in terrains.
		m_Context->View<Component::Terrain>()
			.Each([&](Component::Terrain& terrain)
			{
				terrain.ChunksCache.clear();

				JobSystem::Get().Wait(terrain.ChunkDataContext);
				terrain.ChunkDataQueue.Reset();

				JobSystem::Get().Wait(terrain.ChunkMeshDataContext);
				terrain.ChunkMeshDataQueue.Reset();
			});
	}
}