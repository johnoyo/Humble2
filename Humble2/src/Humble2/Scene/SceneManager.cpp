#include "SceneManager.h"

namespace HBL2
{
	Handle<Scene> SceneManager::LoadScene(Handle<Asset> sceneAssetHandle, bool runtime)
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

		if (!sceneAssetHandle.IsValid())
		{
			HBL2_CORE_ERROR("Handle of scene asset is invalid, aborting scene load.");
			return Handle<Scene>();
		}

		Handle<Scene> sceneHandle = AssetManager::Instance->GetAsset<Scene>(sceneAssetHandle);
		Context::ActiveScene = sceneHandle;

		Scene* scene = ResourceManager::Instance->GetScene(sceneHandle);

		if (scene == nullptr)
		{
			HBL2_CORE_ERROR("Scene asset is invalid, aborting scene load.");
			return Handle<Scene>();
		}

		if (runtime)
		{
			for (ISystem* system : scene->GetSystems())
			{
				system->OnCreate();
			}
		}

		return sceneHandle;
	}
}
