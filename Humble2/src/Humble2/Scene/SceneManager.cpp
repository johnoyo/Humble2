#include "SceneManager.h"

namespace HBL2
{
	SceneManager& SceneManager::Get()
	{
		static SceneManager instance;
		return instance;
	}

	void SceneManager::LoadScene(Handle<Asset> sceneAssetHandle, bool runtime)
	{
		m_NewSceneAssetHandle = sceneAssetHandle;
		m_NewSceneHandle = {};
		m_RuntimeSceneChange = runtime;
		SceneChangeRequested = true;
		m_SceneChangeSource = SceneChangeSource::Asset;
	}

	void SceneManager::LoadScene(Handle<Scene> sceneHandle, bool runtime)
	{
		m_NewSceneHandle = sceneHandle;
		m_NewSceneAssetHandle = {};
		m_RuntimeSceneChange = runtime;
		SceneChangeRequested = true;
		m_SceneChangeSource = SceneChangeSource::Resource;
	}

	void SceneManager::LoadPlaymodeScene(Handle<Scene> sceneHandle, bool runtime)
	{
		m_NewSceneHandle = sceneHandle;
		m_NewSceneAssetHandle = {};
		m_RuntimeSceneChange = runtime;
		SceneChangeRequested = true;
		m_SceneChangeSource = SceneChangeSource::ResourcePlayMode;
	}

	void SceneManager::LoadSceneDeffered()
	{
		if (!AssetManager::Instance->IsAssetValid(m_NewSceneAssetHandle) && !m_NewSceneHandle.IsValid())
		{
			HBL2_CORE_ERROR("Scene asset is invalid, aborting scene load.");
			SceneChangeRequested = false;
			return;
		}

		if (m_CurrentSceneHandle.IsValid())
		{
			Scene* scene = ResourceManager::Instance->GetScene(m_CurrentSceneHandle);

			if (scene != nullptr)
			{
				for (ISystem* system : scene->GetSystems())
				{
					system->OnDestroy();
				}
			}
		}

		switch (m_SceneChangeSource)
		{
		case HBL2::SceneManager::SceneChangeSource::None:
			HBL2_CORE_ASSERT(false, "ERROR: Illegal SceneManager state when changing scenes!");
			return;
		case HBL2::SceneManager::SceneChangeSource::Asset:
			LoadSceneFromAsset();
			return;
		case HBL2::SceneManager::SceneChangeSource::Resource:
			LoadSceneFromResource();
			return;
		case HBL2::SceneManager::SceneChangeSource::ResourcePlayMode:
			LoadSceneFromResourceForPlaymode();
			return;
		}

		HBL2_CORE_ERROR("Scene handle is invalid, aborting scene load.");
		SceneChangeRequested = false;
		return;
	}

	void SceneManager::LoadSceneFromAsset()
	{
		if (!m_NewSceneAssetHandle.IsValid())
		{
			HBL2_CORE_ERROR("Scene asset handle is invalid, aborting scene load.");
			SceneChangeRequested = false;
			return;
		}

		// Delete old loaded scene.
		if (m_CurrentSceneAssetHandle != m_NewSceneAssetHandle)
		{
			AssetManager::Instance->DeleteAsset(m_CurrentSceneAssetHandle);
		}

		// Load new scene.
		Handle<Scene> sceneHandle = AssetManager::Instance->GetAsset<Scene>(m_NewSceneAssetHandle);

		// Trigger scene change event
		EventDispatcher::Get().Post(SceneChangeEvent(m_CurrentSceneHandle, sceneHandle));

		Context::ActiveScene = sceneHandle;

		m_CurrentSceneAssetHandle = m_NewSceneAssetHandle;
		m_CurrentSceneHandle = sceneHandle;

		Scene* scene = ResourceManager::Instance->GetScene(sceneHandle);

		if (scene == nullptr)
		{
			HBL2_CORE_ERROR("Scene asset is invalid, aborting scene load.");
			SceneChangeRequested = false;
			return;
		}

		for (ISystem* system : scene->GetCoreSystems())
		{
			system->OnCreate();
			system->SetState(SystemState::Play);
		}

		if (m_RuntimeSceneChange)
		{
			for (ISystem* system : scene->GetRuntimeSystems())
			{
				system->OnCreate();
				system->SetState(SystemState::Play);
			}
		}

		SceneChangeRequested = false;
		return;
	}

	void SceneManager::LoadSceneFromResource()
	{
		if (!m_NewSceneHandle.IsValid())
		{
			SceneChangeRequested = false;
			HBL2_CORE_ERROR("Scene handle is invalid, aborting scene load.");
			return;
		}

		// Delete old loaded scene.
		if (m_CurrentSceneAssetHandle != m_NewSceneAssetHandle)
		{
			AssetManager::Instance->DeleteAsset(m_CurrentSceneAssetHandle);
		}

		// Load new scene.
		Handle<Scene> sceneHandle = AssetManager::Instance->GetAsset<Scene>(m_NewSceneAssetHandle);

		EventDispatcher::Get().Post(SceneChangeEvent(m_CurrentSceneHandle, m_NewSceneHandle));

		Context::ActiveScene = m_NewSceneHandle;
		m_CurrentSceneHandle = m_NewSceneHandle;
		m_CurrentSceneAssetHandle = {};

		const std::vector<Handle<Asset>>& assetHandles = AssetManager::Instance->GetRegisteredAssets();

		for (auto handle : assetHandles)
		{
			Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
			if (asset->Type == AssetType::Scene && asset->Indentifier != 0 && asset->Indentifier == m_NewSceneHandle.Pack())
			{
				m_CurrentSceneAssetHandle = handle;
				break;
			}
		}

		if (!m_CurrentSceneAssetHandle.IsValid())
		{
			HBL2_CORE_ERROR("Could not resolve new scene asset handle.");
		}

		Scene* scene = ResourceManager::Instance->GetScene(m_NewSceneHandle);

		if (scene == nullptr)
		{
			HBL2_CORE_ERROR("Scene asset is invalid, aborting scene load.");
			SceneChangeRequested = false;
			return;
		}

		for (ISystem* system : scene->GetCoreSystems())
		{
			system->OnCreate();
			system->SetState(SystemState::Play);
		}

		if (m_RuntimeSceneChange)
		{
			for (ISystem* system : scene->GetRuntimeSystems())
			{
				system->OnCreate();
				system->SetState(SystemState::Play);
			}
		}

		SceneChangeRequested = false;
		return;
	}

	void SceneManager::LoadSceneFromResourceForPlaymode()
	{
		if (!m_NewSceneHandle.IsValid())
		{
			SceneChangeRequested = false;
			HBL2_CORE_ERROR("Scene handle is invalid, aborting scene load.");
			return;
		}

		EventDispatcher::Get().Post(SceneChangeEvent(m_CurrentSceneHandle, m_NewSceneHandle));

		Context::ActiveScene = m_NewSceneHandle;
		m_CurrentSceneHandle = m_NewSceneHandle;
		m_CurrentSceneAssetHandle = {};
		m_NewSceneAssetHandle = {};

		Scene* scene = ResourceManager::Instance->GetScene(m_NewSceneHandle);

		if (scene == nullptr)
		{
			HBL2_CORE_ERROR("Scene asset is invalid, aborting scene load.");
			SceneChangeRequested = false;
			return;
		}

		for (ISystem* system : scene->GetCoreSystems())
		{
			system->OnCreate();
			system->SetState(SystemState::Play);
		}

		if (m_RuntimeSceneChange)
		{
			for (ISystem* system : scene->GetRuntimeSystems())
			{
				system->OnCreate();
				system->SetState(SystemState::Play);
			}
		}

		SceneChangeRequested = false;
		return;
	}
}
