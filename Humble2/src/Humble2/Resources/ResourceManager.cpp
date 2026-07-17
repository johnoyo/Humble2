#include "ResourceManager.h"

namespace HBL2
{
	ResourceManager* ResourceManager::Instance = nullptr;

	void ResourceManager::InternalInitialize()
	{
		m_MeshPool.Initialize(m_Spec.Meshes);
		m_MaterialPool.Initialize(m_Spec.Materials);
		m_ScenePool.Initialize(m_Spec.Scenes);
		m_ScriptPool.Initialize(m_Spec.Scripts);
		m_SoundPool.Initialize(m_Spec.Sounds);
		m_PrefabPool.Initialize(m_Spec.Prefabs);
	}

	void ResourceManager::HashCombine(uint64_t& hash, uint64_t value)
	{
		// Golden ratio constant — spreads bits and breaks commutativity
		hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
	}

	const ResourceManagerSpecification& ResourceManager::GetSpec() const
	{
		return m_Spec;
	}

	ResourceDeletionQueue& ResourceManager::GetDeletionQueue()
	{
		return m_DeletionQueue;
	}

	void ResourceManager::Flush(uint64_t currentFrame)
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
	void ResourceManager::ReimportMesh(Handle<Mesh> handle, const MeshDescriptor&& desc)
	{
		Mesh* mesh = m_MeshPool.Get(handle);
		if (mesh != nullptr)
		{
			mesh->Reimport(std::forward<const MeshDescriptor>(desc));
		}
	}
	void ResourceManager::ReimportMesh(Handle<Mesh> handle, const MeshDescriptorEx&& desc)
	{
		Mesh* mesh = m_MeshPool.Get(handle);
		if (mesh != nullptr)
		{
			mesh->Reimport(std::forward<const MeshDescriptorEx>(desc));
		}
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
	void ResourceManager::ReimportMaterial(Handle<Material> handle, const MaterialDescriptor&& desc)
	{
		Material* mat = m_MaterialPool.Get(handle);
		if (mat != nullptr)
		{
			mat->Reimport(std::forward<const MaterialDescriptor>(desc));
		}
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
	uint64_t ResourceManager::GetBindGroupHash(const BindGroupDescriptor&& desc)
	{
		uint64_t hash = 0;

		uint64_t bufferHash = 0x517cc1b727220a95ULL;
		uint64_t textureHash = 0x9e3779b97f4a7c15ULL;

		for (const auto& bufferEntry : desc.buffers)
		{
			HashCombine(bufferHash, bufferEntry.buffer.HashKey() + typeid(Buffer).hash_code());
			HashCombine(bufferHash, bufferEntry.byteOffset);
			HashCombine(bufferHash, bufferEntry.range);
		}

		for (const auto& textureEntry : desc.textures)
		{
			HashCombine(textureHash, textureEntry.texture.HashKey() + typeid(Texture).hash_code());
			HashCombine(textureHash, static_cast<uint64_t>(textureEntry.desiredLayout));
		}

		HashCombine(hash, bufferHash);
		HashCombine(hash, textureHash);
		HashCombine(hash, desc.layout.HashKey());

		return hash;
	}
	uint64_t ResourceManager::GetBindGroupLayoutHash(const BindGroupLayoutDescriptor&& desc)
	{
		uint64_t hash = 0;

		uint64_t bufferHash = 0x517cc1b727220a95ULL;
		uint64_t textureHash = 0x9e3779b97f4a7c15ULL;

		for (const auto& bufferEntry : desc.bufferBindings)
		{
			HashCombine(bufferHash, bufferEntry.slot);
			HashCombine(bufferHash, static_cast<uint64_t>(bufferEntry.type));
			HashCombine(bufferHash, static_cast<uint64_t>(bufferEntry.visibility));
		}

		for (const auto& texture : desc.textureBindings)
		{
			HashCombine(textureHash, texture.slot);
			HashCombine(textureHash, static_cast<uint64_t>(texture.type));
			HashCombine(textureHash, static_cast<uint64_t>(texture.visibility));
		}

		HashCombine(hash, bufferHash);
		HashCombine(hash, textureHash);

		return hash;
	}
}
