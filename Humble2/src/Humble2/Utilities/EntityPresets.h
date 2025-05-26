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

		static entt::entity CreateSkyLight()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a sky light entity.");
				return entt::null;
			}

			auto entity = scene->CreateEntity("New SkyLight");
			scene->AddComponent<HBL2::Component::SkyLight>(entity);
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

			staticMesh.Mesh = MeshUtilities::Get().GetLoadedMeshHandle("assets/meshes/plane.obj");
			staticMesh.Material = AssetManager::Instance->GetAsset<Material>(ShaderUtilities::Get().LitMaterialAsset);

			return entity;
		}

		static entt::entity CreateTessellatedPlane()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a tessellated plane entity.");
				return entt::null;
			}

			auto entity = scene->CreateEntity("TessellatedPlane");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			auto& staticMesh = scene->AddComponent<HBL2::Component::StaticMesh>(entity);

			staticMesh.Mesh = MeshUtilities::Get().GetLoadedMeshHandle("assets/meshes/tessellated_plane.obj");
			staticMesh.Material = AssetManager::Instance->GetAsset<Material>(ShaderUtilities::Get().LitMaterialAsset);

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

			staticMesh.Mesh = MeshUtilities::Get().GetLoadedMeshHandle("assets/meshes/cube.obj");
			staticMesh.Material = AssetManager::Instance->GetAsset<Material>(ShaderUtilities::Get().LitMaterialAsset);

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

			staticMesh.Mesh = MeshUtilities::Get().GetLoadedMeshHandle("assets/meshes/sphere.obj");
			staticMesh.Material = AssetManager::Instance->GetAsset<Material>(ShaderUtilities::Get().LitMaterialAsset);

			return entity;
		}

		static entt::entity CreateCapsule()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a capsule entity.");
				return entt::null;
			}

			auto entity = scene->CreateEntity("Capsule");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			auto& staticMesh = scene->AddComponent<HBL2::Component::StaticMesh>(entity);

			staticMesh.Mesh = MeshUtilities::Get().GetLoadedMeshHandle("assets/meshes/capsule.obj");
			staticMesh.Material = AssetManager::Instance->GetAsset<Material>(ShaderUtilities::Get().LitMaterialAsset);

			return entity;
		}

		static entt::entity CreateCylinder()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a cylinder entity.");
				return entt::null;
			}

			auto entity = scene->CreateEntity("Cylinder");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			auto& staticMesh = scene->AddComponent<HBL2::Component::StaticMesh>(entity);

			staticMesh.Mesh = MeshUtilities::Get().GetLoadedMeshHandle("assets/meshes/cylinder.obj");
			staticMesh.Material = AssetManager::Instance->GetAsset<Material>(ShaderUtilities::Get().LitMaterialAsset);

			return entity;
		}

		static entt::entity CreateTorus()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a torus entity.");
				return entt::null;
			}

			auto entity = scene->CreateEntity("Torus");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			auto& staticMesh = scene->AddComponent<HBL2::Component::StaticMesh>(entity);

			staticMesh.Mesh = MeshUtilities::Get().GetLoadedMeshHandle("assets/meshes/torus.obj");
			staticMesh.Material = AssetManager::Instance->GetAsset<Material>(ShaderUtilities::Get().LitMaterialAsset);

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