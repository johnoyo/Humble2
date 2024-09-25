#include "SceneManager.h"

namespace HBL2
{
	void SceneManager::LoadScene(Handle<Asset> sceneAssetHandle, bool runtime)
	{
		m_NewSceneAssetHandle = sceneAssetHandle;
		m_NewSceneHandle = {};
		m_RuntimeSceneChange = runtime;
		SceneChangeRequested = true;
	}

	void SceneManager::LoadScene(Handle<Scene> sceneHandle, bool runtime)
	{
		m_NewSceneHandle = sceneHandle;
		m_NewSceneAssetHandle = {};
		m_RuntimeSceneChange = runtime;
		SceneChangeRequested = true;
	}

	void SceneManager::LoadSceneDeffered()
	{
		if (Context::ActiveScene.IsValid())
		{
			Scene* scene = ResourceManager::Instance->GetScene(Context::ActiveScene);

			if (scene != nullptr)
			{
				for (ISystem* system : scene->GetSystems())
				{
					system->OnDestroy();
				}
			}
		}

		if (m_NewSceneAssetHandle.IsValid())
		{
			Handle<Scene> sceneHandle = AssetManager::Instance->GetAsset<Scene>(m_NewSceneAssetHandle);

			EventDispatcher::Get().Post(SceneChangeEvent(Context::ActiveScene, sceneHandle));

			Context::ActiveScene = sceneHandle;

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

		if (m_NewSceneHandle.IsValid())
		{
			EventDispatcher::Get().Post(SceneChangeEvent(Context::ActiveScene, m_NewSceneHandle));

			Context::ActiveScene = m_NewSceneHandle;

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

		HBL2_CORE_ERROR("Scene handle is invalid, aborting scene load.");
		SceneChangeRequested = false;
		return;
	}
}
