#include "SceneManager.h"

#include "ISystem.h"

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

		Handle<Scene> oldSceneHandle = m_CurrentSceneHandle;
		Handle<Asset> oldSceneAssetHandle = m_CurrentSceneAssetHandle;

		// Load new scene.
		Handle<Scene> newSceneHandle = LoadNewScene();

		// Emit scene change event.
		EventDispatcher::Get().Post(SceneChangeEvent(m_CurrentSceneHandle, newSceneHandle));

		// State management.
		ManageSceneChangeState();

		// Enable new scene systems.
		EnableNewSceneSystems(newSceneHandle);

		// Unload old scene.
		UnloadOldScene(oldSceneHandle, oldSceneAssetHandle);

		SceneChangeRequested = false;
		return;
	}

	Handle<Scene> SceneManager::LoadNewScene() const
	{
		switch (m_SceneChangeSource)
		{
		case HBL2::SceneManager::SceneChangeSource::None:
			HBL2_CORE_ASSERT(false, "ERROR: Illegal SceneManager state when changing scenes!");
			return {};
		case HBL2::SceneManager::SceneChangeSource::Asset:
			return AssetManager::Instance->GetAsset<Scene>(m_NewSceneAssetHandle);
		case HBL2::SceneManager::SceneChangeSource::Resource:
		case HBL2::SceneManager::SceneChangeSource::ResourcePlayMode:
			return m_NewSceneHandle;
		}

		return {};
	}

	void SceneManager::ManageSceneChangeState()
	{
		switch (m_SceneChangeSource)
		{
		case HBL2::SceneManager::SceneChangeSource::None:
			HBL2_CORE_ASSERT(false, "ERROR: Illegal SceneManager state when changing scenes!");
			return;
		case HBL2::SceneManager::SceneChangeSource::Asset:
			{
				// Get scene handle.
				Handle<Scene> sceneHandle = AssetManager::Instance->GetAsset<Scene>(m_NewSceneAssetHandle);

				// Set active and current scene handles.
				Context::ActiveScene = sceneHandle;
				m_CurrentSceneAssetHandle = m_NewSceneAssetHandle;
				m_CurrentSceneHandle = sceneHandle;
			}
			return;
		case HBL2::SceneManager::SceneChangeSource::Resource:
			{
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
			}
			return;
		case HBL2::SceneManager::SceneChangeSource::ResourcePlayMode:
			{
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
					// NOTE: This is needed since if in playmode we change the scene, and then when we exit play mode and change scenes,
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
			}
			return;
		}
	}

	void SceneManager::EnableNewSceneSystems(Handle<Scene> newSceneHandle) const
	{
		Scene* newScene = ResourceManager::Instance->GetScene(newSceneHandle);

		if (newScene == nullptr)
		{
			HBL2_CORE_ERROR("Scene asset is invalid, aborting scene load.");
			return;
		}

		// Abort enabling the systems of the scene that was used to enter play mode, since it was not unloaded.
		if (m_SceneChangeSource == SceneChangeSource::ResourcePlayMode && newScene->GetName().find("(Clone)") == StaticString<64>::npos)
		{
			return;
		}

		for (ISystem* system : newScene->GetCoreSystems())
		{
			system->OnAttach();
			system->SetState(SystemState::Play);
		}

		if (m_RuntimeSceneChange)
		{
			for (ISystem* system : newScene->GetRuntimeSystems())
			{
				system->OnAttach();
				system->SetState(SystemState::Play);
			}
		}

		for (ISystem* system : newScene->GetCoreSystems())
		{
			system->OnCreate();
			system->SetState(SystemState::Play);
		}

		if (m_RuntimeSceneChange)
		{
			for (ISystem* system : newScene->GetRuntimeSystems())
			{
				system->OnCreate();
				system->SetState(SystemState::Play);
			}
		}
	}

	void SceneManager::UnloadOldScene(Handle<Scene> oldSceneHandle, Handle<Asset> oldSceneAssetHandle) const
	{
		// Delete old loaded scene.
		switch (m_SceneChangeSource)
		{
		case SceneChangeSource::Asset:
		case SceneChangeSource::Resource:
			{
				// Unload old scene systems.
				if (oldSceneHandle.IsValid())
				{
					Scene* scene = ResourceManager::Instance->GetScene(oldSceneHandle);

					if (scene != nullptr)
					{
						for (ISystem* system : scene->GetSystems())
						{
							system->OnDestroy();
						}

						for (ISystem* system : scene->GetSystems())
						{
							system->OnDetach();
						}
					}
				}

				// Abort unload if its the same as the new one.
				if (oldSceneAssetHandle != m_NewSceneAssetHandle)
				{
					// If we are in play mode and we changes scenes, dot not delete the scene that was played.
					if (oldSceneAssetHandle != m_BaseSceneAssetHandle)
					{
						AssetManager::Instance->DeleteAsset(oldSceneAssetHandle);
					}
				}
			}
			break;
		case SceneChangeSource::ResourcePlayMode:
			{
				// Abort unload if its not a temporary playmode scene.
				// We dont want to delete the old scene when we enter play mode,
				// since we want to go back to it in the state that it was before entering play mode.
				Scene* oldScene = ResourceManager::Instance->GetScene(oldSceneHandle);
				if (oldScene != nullptr && oldScene->GetName().find("(Clone)") != StaticString<64>::npos)
				{
					// Unload old scene systems.
					if (oldSceneHandle.IsValid())
					{
						Scene* scene = ResourceManager::Instance->GetScene(oldSceneHandle);

						if (scene != nullptr)
						{
							for (ISystem* system : scene->GetSystems())
							{
								system->OnDestroy();
							}

							for (ISystem* system : scene->GetSystems())
							{
								system->OnDetach();
							}
						}
					}

					// Clear entire scene.
					oldScene->Clear();

					// Delete play mode scene.
					ResourceManager::Instance->DeleteScene(oldSceneHandle);
				}
			}
			break;
		}

	}
}
