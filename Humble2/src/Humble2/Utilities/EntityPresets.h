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
			auto& staticMesh = scene->AddComponent<HBL2::Component::StaticMesh_New>(entity);

			float vertexBuffer[] =
			{
				-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
				 0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
				 0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
				 0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
			    -0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
			    -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
			};

			auto buffer = ResourceManager::Instance->CreateBuffer({
				.debugName = "plane_vertex_buffer",
				.byteSize = sizeof(float) * 48,
				.initialData = vertexBuffer,
			});

			auto planeMesh = ResourceManager::Instance->CreateMesh({
				.debugName = "plane_mesh",
				.vertexOffset = 0,
				.vertexCount = 6,
				.vertexBuffers = { buffer },
			});

			staticMesh.Mesh = planeMesh;

			return entity;
		}

		static entt::entity CreateCube()
		{
			return entt::null;
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
			auto& staticMesh = scene->AddComponent<HBL2::Component::StaticMesh_New>(entity);

			std::vector<float> vertexBuffer;
			std::vector<uint32_t> indices;

			uint32_t xSegments = 8;
			uint32_t ySegments = 8;

			for (uint32_t x = 0; x <= xSegments; x++)
			{
				for (uint32_t y = 0; y <= ySegments; y++)
				{
					float xSeg = (float)x / (float)xSegments;
					float ySeg = (float)y / (float)ySegments;

					float xPos = glm::cos(xSeg * 2.0f * glm::pi<float>()) * glm::sin(ySeg * glm::pi<float>());
					float yPos = glm::cos(ySeg * glm::pi<float>());
					float zPos = glm::sin(xSeg * 2.0f * glm::pi<float>()) * glm::sin(ySeg * glm::pi<float>());

					vertexBuffer.insert(vertexBuffer.end(), { xPos, yPos, zPos, xSeg, ySeg, xPos, yPos, zPos });
				}
			}

			bool oddRow = false;

			for (uint32_t y = 0; y < ySegments; y++)
			{
				if (!oddRow)
				{
					for (uint32_t x = 0; x <= xSegments; x++)
					{
						indices.push_back(y * (xSegments + 1) + x);
						indices.push_back((y + 1) * (xSegments + 1) + x);
					}
				}
				else
				{
					for (uint32_t x = xSegments; x >= 0; x--)
					{
						indices.push_back((y + 1) * (xSegments + 1) + x);
						indices.push_back(y * (xSegments + 1) + x);
					}
				}
			}

			auto buffer = ResourceManager::Instance->CreateBuffer({
				.debugName = "plane_vertex_buffer",
				.byteSize = (uint32_t)sizeof(float) * (uint32_t)vertexBuffer.size(),
				.initialData = vertexBuffer.data(),
			});

			auto indexBuffer = ResourceManager::Instance->CreateBuffer({
				.debugName = "plane_index_buffer",
				.byteSize = (uint32_t)sizeof(float) * (uint32_t)vertexBuffer.size(),
				.initialData = vertexBuffer.data(),
			});

			auto planeMesh = ResourceManager::Instance->CreateMesh({
				.debugName = "plane_mesh",
				.indexOffset = 0,
				.indexCount = (uint32_t)indices.size(),
				.vertexOffset = 0,
				.vertexCount = (uint32_t)vertexBuffer.size() / 8,
				.indexBuffer = indexBuffer,
				.vertexBuffers = { buffer },
			});

			staticMesh.Mesh = planeMesh;

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
			scene->AddComponent<HBL2::Component::Sprite_New>(entity);

			return entity;
		}
	}
}