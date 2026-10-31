#include "EntityPresets.h"

#include "Core/Context.h"
#include "Resources/ResourceManager.h"
#include "Utilities/ShaderUtilities.h"
#include "Utilities/MeshUtilities.h"

namespace HBL2
{
	Entity EntityPreset::CreateEmpty()
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
	
	Entity EntityPreset::CreateCamera()
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
	
	Entity EntityPreset::CreateLight()
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
	
	Entity EntityPreset::CreateSkyLight()
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
	
	Entity EntityPreset::CreatePlane()
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

		staticMesh.Mesh = MeshUtilities::Get().GetBuiltInLoadedMeshAssetHandle(BuiltInMesh::PLANE);
		staticMesh.Material = ShaderUtilities::Get().LitMaterialAsset;

		return entity;
	}
	
	Entity EntityPreset::CreateTessellatedPlane()
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

		staticMesh.Mesh = MeshUtilities::Get().GetBuiltInLoadedMeshAssetHandle(BuiltInMesh::TESSELATED_PLANE);
		staticMesh.Material = ShaderUtilities::Get().LitMaterialAsset;

		return entity;
	}
	
	Entity EntityPreset::CreateCube()
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

		staticMesh.Mesh = MeshUtilities::Get().GetBuiltInLoadedMeshAssetHandle(BuiltInMesh::CUBE);
		staticMesh.Material = ShaderUtilities::Get().LitMaterialAsset;

		return entity;
	}
	
	Entity EntityPreset::CreateSphere()
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

		staticMesh.Mesh = MeshUtilities::Get().GetBuiltInLoadedMeshAssetHandle(BuiltInMesh::SPHERE);
		staticMesh.Material = ShaderUtilities::Get().LitMaterialAsset;

		return entity;
	}
	
	Entity EntityPreset::CreateCapsule()
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

		staticMesh.Mesh = MeshUtilities::Get().GetBuiltInLoadedMeshAssetHandle(BuiltInMesh::CAPSULE);
		staticMesh.Material = ShaderUtilities::Get().LitMaterialAsset;

		return entity;
	}
	
	Entity EntityPreset::CreateCylinder()
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

		staticMesh.Mesh = MeshUtilities::Get().GetBuiltInLoadedMeshAssetHandle(BuiltInMesh::CYLINDER);
		staticMesh.Material = ShaderUtilities::Get().LitMaterialAsset;

		return entity;
	}
	
	Entity EntityPreset::CreateTorus()
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

		staticMesh.Mesh = MeshUtilities::Get().GetBuiltInLoadedMeshAssetHandle(BuiltInMesh::TORUS);
		staticMesh.Material = ShaderUtilities::Get().LitMaterialAsset;

		return entity;
	}
	
	Entity EntityPreset::CreateSprite()
	{
		Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		if (scene == nullptr)
		{
			HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a sprite entity.");
			return Entity::Null;
		}

		auto entity = scene->CreateEntity("Sprite");
		scene->AddComponent<HBL2::Component::EditorVisible>(entity);
		auto& sprite = scene->AddComponent<HBL2::Component::Sprite>(entity);
		sprite.Material = ShaderUtilities::Get().UnlitMaterialAsset;

		return entity;
	}
	
	Entity EntityPreset::CreateTerrain()
	{
		Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		if (scene == nullptr)
		{
			HBL2_CORE_ERROR("Could not retrieve ActiveScene when creating a terrain entity.");
			return Entity::Null;
		}

		auto entity = scene->CreateEntity("Terrain");
		scene->AddComponent<HBL2::Component::EditorVisible>(entity);

		scene->AddComponent<HBL2::Component::AnimationCurve>(entity);
		scene->AddComponent<HBL2::Component::Terrain>(entity);

		return entity;
	}
}