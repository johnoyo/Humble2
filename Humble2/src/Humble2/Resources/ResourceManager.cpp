#include "ResourceManager.h"

namespace HBL2
{
	ResourceManager* ResourceManager::Instance = nullptr;

	void ResourceManager::Flush(uint32_t currentFrame)
	{
		m_DeletionQueue.Flush(currentFrame);
	}

	void ResourceManager::FlushAll()
	{
		m_DeletionQueue.FlushAll();
	}

	// Mesh
	Handle<Mesh> ResourceManager::CreateMesh(const MeshDescriptorEx&& desc)
	{
		return m_MeshPool.Insert(std::forward<const MeshDescriptorEx>(desc));
	}
	Handle<Mesh> ResourceManager::CreateMesh(const MeshDescriptor&& desc)
	{
		return m_MeshPool.Insert(std::forward<const MeshDescriptor>(desc));
	}
	void ResourceManager::DeleteMesh(Handle<Mesh> handle)
	{
		m_MeshPool.Remove(handle);
	}
	Mesh* ResourceManager::GetMesh(Handle<Mesh> handle) const
	{
		return m_MeshPool.Get(handle);
	}

	// Materials
	Handle<Material> ResourceManager::CreateMaterial(const MaterialDescriptor&& desc)
	{
		return m_MaterialPool.Insert(std::forward<const MaterialDescriptor>(desc));
	}
	void ResourceManager::DeleteMaterial(Handle<Material> handle)
	{
		m_MaterialPool.Remove(handle);
	}
	Material* ResourceManager::GetMaterial(Handle<Material> handle) const
	{
		return m_MaterialPool.Get(handle);
	}

	// Scenes
	Handle<Scene> ResourceManager::CreateScene(const SceneDescriptor&& desc)
	{
		return m_ScenePool.Insert(std::forward<const SceneDescriptor>(desc));
	}
	void ResourceManager::DeleteScene(Handle<Scene> handle)
	{
		m_ScenePool.Remove(handle);
	}
	Scene* ResourceManager::GetScene(Handle<Scene> handle) const
	{
		return m_ScenePool.Get(handle);
	}

	// Scripts
	Handle<Script> ResourceManager::ResourceManager::CreateScript(const ScriptDescriptor&& desc)
	{
		return m_ScriptPool.Insert(std::forward<const ScriptDescriptor>(desc));
	}
	void ResourceManager::DeleteScript(Handle<Script> handle)
	{
		m_ScriptPool.Remove(handle);
	}
	Script* ResourceManager::GetScript(Handle<Script> handle) const
	{
		return m_ScriptPool.Get(handle);
	}

	// Sounds
	Handle<Sound> ResourceManager::CreateSound(const SoundDescriptor&& desc)
	{
		return m_SoundPool.Insert(std::forward<const SoundDescriptor>(desc));
	}
	void ResourceManager::DeleteSound(Handle<Sound> handle)
	{
		m_SoundPool.Remove(handle);
	}
	Sound* ResourceManager::GetSound(Handle<Sound> handle) const
	{
		return m_SoundPool.Get(handle);
	}

	// Prefabs
	Handle<Prefab> ResourceManager::CreatePrefab(const PrefabDescriptor&& desc)
	{
		return m_PrefabPool.Insert(std::forward<const PrefabDescriptor>(desc));
	}
	void ResourceManager::DeletePrefab(Handle<Prefab> handle)
	{
		m_PrefabPool.Remove(handle);
	}
	Prefab* ResourceManager::GetPrefab(Handle<Prefab> handle) const
	{
		return m_PrefabPool.Get(handle);
	}
}