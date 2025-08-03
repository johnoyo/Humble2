#pragma once

#include <Core\Context.h>
#include <Resources\ResourceManager.h>
#include <Utilities\ShaderUtilities.h>
#include <Utilities\MeshUtilities.h>

#include <entt\include\entt.hpp>

namespace HBL2
{
	namespace EntityPreset
	{
		static Entity CreateEmpty()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating an empty entity.");
				return Entity::Null;
			}

			auto entity = scene->CreateEntity();
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);

			return entity;
		}

		static Entity CreateCamera()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a camera entity.");
				return Entity::Null;
			}

			auto entity = scene->CreateEntity("Camera");
			scene->AddComponent<HBL2::Component::Camera>(entity).Enabled = true;
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			scene->GetComponent<HBL2::Component::Transform>(entity).Translation.z = 10.f;

			return entity;
		}

		static Entity CreateLight()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a light entity.");
				return Entity::Null;
			}

			auto entity = scene->CreateEntity("New Light");
			scene->AddComponent<HBL2::Component::Light>(entity);
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);

			return entity;
		}

		static Entity CreateSkyLight()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a sky light entity.");
				return Entity::Null;
			}

			auto entity = scene->CreateEntity("New SkyLight");
			scene->AddComponent<HBL2::Component::SkyLight>(entity);
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);

			return entity;
		}

		static Entity CreatePlane()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a plane entity.");
				return Entity::Null;
			}

			auto entity = scene->CreateEntity("Plane");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			auto& staticMesh = scene->AddComponent<HBL2::Component::StaticMesh>(entity);

			staticMesh.Mesh = MeshUtilities::Get().GetLoadedMeshHandle("assets/meshes/plane.obj");
			staticMesh.Material = AssetManager::Instance->GetAsset<Material>(ShaderUtilities::Get().LitMaterialAsset);

			return entity;
		}

		static Entity CreateTessellatedPlane()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a tessellated plane entity.");
				return Entity::Null;
			}

			auto entity = scene->CreateEntity("TessellatedPlane");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			auto& staticMesh = scene->AddComponent<HBL2::Component::StaticMesh>(entity);

			staticMesh.Mesh = MeshUtilities::Get().GetLoadedMeshHandle("assets/meshes/tessellated_plane.obj");
			staticMesh.Material = AssetManager::Instance->GetAsset<Material>(ShaderUtilities::Get().LitMaterialAsset);

			return entity;
		}

		static Entity CreateCube()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a cube entity.");
				return Entity::Null;
			}

			auto entity = scene->CreateEntity("Cube");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			auto& staticMesh = scene->AddComponent<HBL2::Component::StaticMesh>(entity);

			staticMesh.Mesh = MeshUtilities::Get().GetLoadedMeshHandle("assets/meshes/cube.obj");
			staticMesh.Material = AssetManager::Instance->GetAsset<Material>(ShaderUtilities::Get().LitMaterialAsset);

			return entity;
		}

		static Entity CreateSphere()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a sphere entity.");
				return Entity::Null;
			}

			auto entity = scene->CreateEntity("Sphere");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			auto& staticMesh = scene->AddComponent<HBL2::Component::StaticMesh>(entity);

			staticMesh.Mesh = MeshUtilities::Get().GetLoadedMeshHandle("assets/meshes/sphere.obj");
			staticMesh.Material = AssetManager::Instance->GetAsset<Material>(ShaderUtilities::Get().LitMaterialAsset);

			return entity;
		}

		static Entity CreateCapsule()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a capsule entity.");
				return Entity::Null;
			}

			auto entity = scene->CreateEntity("Capsule");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			auto& staticMesh = scene->AddComponent<HBL2::Component::StaticMesh>(entity);

			staticMesh.Mesh = MeshUtilities::Get().GetLoadedMeshHandle("assets/meshes/capsule.obj");
			staticMesh.Material = AssetManager::Instance->GetAsset<Material>(ShaderUtilities::Get().LitMaterialAsset);

			return entity;
		}

		static Entity CreateCylinder()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a cylinder entity.");
				return Entity::Null;
			}

			auto entity = scene->CreateEntity("Cylinder");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			auto& staticMesh = scene->AddComponent<HBL2::Component::StaticMesh>(entity);

			staticMesh.Mesh = MeshUtilities::Get().GetLoadedMeshHandle("assets/meshes/cylinder.obj");
			staticMesh.Material = AssetManager::Instance->GetAsset<Material>(ShaderUtilities::Get().LitMaterialAsset);

			return entity;
		}

		static Entity CreateTorus()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a torus entity.");
				return Entity::Null;
			}

			auto entity = scene->CreateEntity("Torus");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			auto& staticMesh = scene->AddComponent<HBL2::Component::StaticMesh>(entity);

			staticMesh.Mesh = MeshUtilities::Get().GetLoadedMeshHandle("assets/meshes/torus.obj");
			staticMesh.Material = AssetManager::Instance->GetAsset<Material>(ShaderUtilities::Get().LitMaterialAsset);

			return entity;
		}
	
		static Entity CreateSprite()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a sprite entity.");
				return Entity::Null;
			}

			auto entity = scene->CreateEntity("Sprite");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);
			scene->AddComponent<HBL2::Component::Sprite>(entity);

			return entity;
		}

		static Entity CreateTerrain()
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene == nullptr)
			{
				HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a terrain entity.");
				return Entity::Null;
			}

			auto entity = scene->CreateEntity("Terrain");
			scene->AddComponent<HBL2::Component::EditorVisible>(entity);

			scene->AddComponent<HBL2::Component::Link>(entity);
			scene->AddComponent<HBL2::Component::StaticMesh>(entity);
			scene->AddComponent<HBL2::Component::AnimationCurve>(entity);
			scene->AddComponent<HBL2::Component::Terrain>(entity);

			return entity;
		}
	}
}