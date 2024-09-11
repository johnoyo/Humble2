#include "SceneManager.h"

namespace HBL2
{
	Handle<Scene> SceneManager::LoadScene(Handle<Asset> sceneAssetHandle, bool runtime)
	{
		if (!sceneAssetHandle.IsValid())
		{
			return Handle<Scene>();
		}

		Handle<Scene> sceneHandle = AssetManager::Instance->GetAsset<Scene>(sceneAssetHandle);

		if (runtime)
		{
			Scene* scene = ResourceManager::Instance->GetScene(sceneHandle);

			if (scene != nullptr)
			{
				for (ISystem* system : scene->GetSystems())
				{
					system->OnCreate();
				}
			}
		}

		return sceneHandle;
	}
}
