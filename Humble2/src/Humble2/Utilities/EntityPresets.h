#pragma once

#include <Core\Context.h>
#include <Resources\ResourceManager.h>

#include <entt\include\entt.hpp>

namespace HBL2
{
	namespace EntityPreset
	{
		static entt::entity CreateEmpty()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating an empty entity.");
				return entt::null;
			}

			auto entity = scene->CreateEntity();
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);

			return entity;
		}

		static entt::entity CreateCamera()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a camera entity.");
				return entt::null;
			}

			auto entity = scene->CreateEntity("Camera");
			scene->AddComponent<HBL2::Component::Camera>(entity).Enabled = true;
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			scene->GetComponent<HBL2::Component::Transform>(entity).Translation.z = 10.f;

			return entity;
		}

		static entt::entity CreateLight()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a light entity.");
				return entt::null;
			}

			auto entity = scene->CreateEntity("New Light");
			scene->AddComponent<HBL2::Component::Light>(entity);
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);

			return entity;
		}

		static entt::entity CreatePlane()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a plane entity.");
				return entt::null;
			}

			auto entity = scene->CreateEntity("Plane");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			auto& staticMesh = scene->AddComponent<HBL2::Component::StaticMesh>(entity);

			float vertexBuffer[] =
			{
				// position         // normal         // texcoord
				-0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
				 0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
				 0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,

				 0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
				-0.5f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
				-0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
			};


			auto buffer = ResourceManager::Instance->CreateBuffer({
				.debugName = "plane-vertex-buffer",
				.usage = BufferUsage::VERTEX,
				.byteSize = sizeof(float) * 48,
				.initialData = vertexBuffer,
			});

			auto planeMesh = ResourceManager::Instance->CreateMesh({
				.debugName = "plane-mesh",
				.meshes = {
					{
						.debugName = "plane-sub-mesh",
						.subMeshes = {
							{
								.vertexOffset = 0,
								.vertexCount = 6,
								.minVertex = { -0.5f, -0.5f, 0.0f },
								.maxVertex = {  0.5f,  0.5f, 0.0f },
							}
						},
						.vertexBuffers = { buffer },
					}
				}
			});

			staticMesh.Mesh = planeMesh;

			return entity;
		}

		static entt::entity CreateCube()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a cube entity.");
				return entt::null;
			}

			auto entity = scene->CreateEntity("Cube");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			auto& staticMesh = scene->AddComponent<HBL2::Component::StaticMesh>(entity);

			float vertexBuffer[] =
			{
				// positions          // normals           // texture coords

				// Back face
				-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
				 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
				 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,

				 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
				-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
				-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,

				// Front face
				-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
				 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
				 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,

				 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
				-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
				-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

				// Left face
				-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
				-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
				-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,

				-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
				-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
				-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

				// Right face
				 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
				 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
				 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,

				 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
				 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
				 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,

				// Bottom face
				-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
				 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
				 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,

				 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
				-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
				-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

				// Top face
				-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
				 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
				 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,

				 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
				-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
				-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
			};

			auto buffer = ResourceManager::Instance->CreateBuffer({
				.debugName = "cube-vertex-buffer",
				.usage = BufferUsage::VERTEX,
				.byteSize = sizeof(float) * 288,
				.initialData = vertexBuffer,
			});

			auto cubeMesh = ResourceManager::Instance->CreateMesh({
				.debugName = "cube-mesh",
				.meshes = {
					{
						.debugName = "cube-sub-mesh",
						.subMeshes = {
							{
								.vertexOffset = 0,
								.vertexCount = 36,
								.minVertex = { -0.5f, -0.5f, -0.5f },
								.maxVertex = {  0.5f,  0.5f,  0.5f },
							}
						},
						.vertexBuffers = { buffer },
					}
				}
			});

			staticMesh.Mesh = cubeMesh;

			return entity;
		}

		static entt::entity CreateSphere()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a sphere entity.");
				return entt::null;
			}

			auto entity = scene->CreateEntity("Sphere");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			auto& staticMesh = scene->AddComponent<HBL2::Component::StaticMesh>(entity);

			std::vector<float> vertexBuffer;
			std::vector<uint32_t> indices;

			uint32_t xSegments = 32;
			uint32_t ySegments = 16;

			for (uint32_t y = 0; y <= ySegments; ++y)
			{
				for (uint32_t x = 0; x <= xSegments; ++x)
				{
					float xSeg = (float)x / (float)xSegments;
					float ySeg = (float)y / (float)ySegments;

					float xPos = glm::cos(xSeg * 2.0f * glm::pi<float>()) * glm::sin(ySeg * glm::pi<float>());
					float yPos = glm::cos(ySeg * glm::pi<float>());
					float zPos = glm::sin(xSeg * 2.0f * glm::pi<float>()) * glm::sin(ySeg * glm::pi<float>());

					vertexBuffer.insert(vertexBuffer.end(), {
						xPos, yPos, zPos,    // position
						xPos, yPos, zPos,    // normal (same as pos for unit sphere)
						xSeg, ySeg,          // UV
					});
				}
			}

			for (uint32_t y = 0; y < ySegments; ++y)
			{
				for (uint32_t x = 0; x < xSegments; ++x)
				{
					uint32_t i0 = y * (xSegments + 1) + x;
					uint32_t i1 = (y + 1) * (xSegments + 1) + x;
					uint32_t i2 = (y + 1) * (xSegments + 1) + (x + 1);
					uint32_t i3 = y * (xSegments + 1) + (x + 1);

					// Triangle 1
					indices.push_back(i0);
					indices.push_back(i2);
					indices.push_back(i1);

					// Triangle 2
					indices.push_back(i0);
					indices.push_back(i3);
					indices.push_back(i2);
				}
			}

			// Compute AABB
			glm::vec3 minVertex = glm::vec3(std::numeric_limits<float>::max());
			glm::vec3 maxVertex = glm::vec3(std::numeric_limits<float>::lowest());

			for (size_t i = 0; i < vertexBuffer.size(); i += 8)
			{
				glm::vec3 pos = { vertexBuffer[i], vertexBuffer[i + 1], vertexBuffer[i + 2] };
				minVertex = glm::min(minVertex, pos);
				maxVertex = glm::max(maxVertex, pos);
			}

			auto buffer = ResourceManager::Instance->CreateBuffer({
				.debugName = "sphere_vertex_buffer",
				.usage = BufferUsage::VERTEX,
				.byteSize = (uint32_t)sizeof(float) * (uint32_t)vertexBuffer.size(),
				.initialData = vertexBuffer.data(),
			});

			auto indexBuffer = ResourceManager::Instance->CreateBuffer({
				.debugName = "sphere_index_buffer",
				.usage = BufferUsage::INDEX,
				.byteSize = (uint32_t)sizeof(uint32_t) * (uint32_t)indices.size(),
				.initialData = indices.data(),
			});

			auto sphereMesh = ResourceManager::Instance->CreateMesh({
				.debugName = "sphere-mesh",
				.meshes = {
					{
						.debugName = "sphere-sub-mesh",
						.subMeshes = {
							{
								.indexOffset = 0,
								.indexCount = (uint32_t)indices.size(),
								.vertexOffset = 0,
								.vertexCount = (uint32_t)vertexBuffer.size() / 8,
								.minVertex = minVertex,
								.maxVertex = maxVertex,
							}
						},
						.indexBuffer = indexBuffer,
						.vertexBuffers = { buffer },
					}
				}
			});

			staticMesh.Mesh = sphereMesh;

			return entity;
		}
	
		static entt::entity CreateSprite()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a sprite entity.");
				return entt::null;
			}

			auto entity = scene->CreateEntity("Sprite");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			scene->AddComponent<HBL2::Component::Sprite>(entity);

			return entity;
		}
	}
}