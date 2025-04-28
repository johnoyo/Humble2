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

		// Delete old loaded scene, if its not the same as the new one.
		if (m_CurrentSceneAssetHandle != m_NewSceneAssetHandle)
		{
			// If we are in play mode and we changes scenes, dot not delete the scene that was played.
			if (m_CurrentSceneAssetHandle != m_BaseSceneAssetHandle)
			{
				AssetManager::Instance->DeleteAsset(m_CurrentSceneAssetHandle);
			}
		}

		// Load new scene.
		Handle<Scene> sceneHandle = AssetManager::Instance->GetAsset<Scene>(m_NewSceneAssetHandle);

		// Trigger scene change event
		EventDispatcher::Get().Post(SceneChangeEvent(m_CurrentSceneHandle, sceneHandle));

		// Set active and current scene handles.
		Context::ActiveScene = sceneHandle;
		m_CurrentSceneAssetHandle = m_NewSceneAssetHandle;
		m_CurrentSceneHandle = sceneHandle;

		// Get scene object.
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

		// Delete old loaded scene, if its not the same as the new one.
		if (m_CurrentSceneAssetHandle != m_NewSceneAssetHandle && m_NewSceneAssetHandle.IsValid())
		{
			// If we are in play mode and we changes scenes, dot not delete the scene that was played.
			if (m_CurrentSceneAssetHandle != m_BaseSceneAssetHandle)
			{
				AssetManager::Instance->DeleteAsset(m_CurrentSceneAssetHandle);
			}
		}

		// Transmit scene change event.
		EventDispatcher::Get().Post(SceneChangeEvent(m_CurrentSceneHandle, m_NewSceneHandle));

		// Set current and active scene handle.
		Context::ActiveScene = m_NewSceneHandle;
		m_CurrentSceneHandle = m_NewSceneHandle;

		// Find scene asset handle.
		m_CurrentSceneAssetHandle = {};

		for (auto handle : AssetManager::Instance->GetRegisteredAssets())
		{
			Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
			if (asset->Type == AssetType::Scene && asset->Indentifier != 0 && asset->Indentifier == m_NewSceneHandle.Pack())
			{
				m_CurrentSceneAssetHandle = handle;
				break;
			}
		}

		// Make sure the scene asset is found.
		HBL2_CORE_ASSERT(m_CurrentSceneAssetHandle.IsValid(), "Could not resolve new scene asset handle.");

		// Get the scene object.
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

		// Transmit scene change event.
		EventDispatcher::Get().Post(SceneChangeEvent(m_CurrentSceneHandle, m_NewSceneHandle));

		if (m_RuntimeSceneChange)
		{
			// Find the scene asset handle to store it and use when leaving play mode.
			m_CurrentSceneAssetHandle = {};

			for (auto handle : AssetManager::Instance->GetRegisteredAssets())
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->Type == AssetType::Scene && asset->Indentifier != 0 && asset->Indentifier == m_CurrentSceneHandle.Pack())
				{
					m_CurrentSceneAssetHandle = handle;
					break;
				}
			}

			m_BaseSceneAssetHandle = m_CurrentSceneAssetHandle;
		}
		else
		{
			// Find the current scene asset handle.
			// NOTE: This is needed since if in playmode we change the scene, and then when we exit play mode and cahnge scenes,
			//		 the scene that was changed in play mode will be present, so the wrong scene will be unloaded.
			m_CurrentSceneAssetHandle = {};

			for (auto handle : AssetManager::Instance->GetRegisteredAssets())
			{
				Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);
				if (asset->Type == AssetType::Scene && asset->Indentifier != 0 && asset->Indentifier == m_NewSceneHandle.Pack())
				{
					m_CurrentSceneAssetHandle = handle;
					break;
				}
			}

			// Clear the scene when leaving play mode.
			m_BaseSceneAssetHandle = {};
		}

		// Set current and active scene handle.
		Context::ActiveScene = m_NewSceneHandle;
		m_CurrentSceneHandle = m_NewSceneHandle;
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
