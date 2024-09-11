#pragma once

#include "Scene\Scene.h"
#include "Scene\SceneSerializer.h"
#include "Resources\Handle.h"

#include "Asset\AssetManager.h"
#include "Resources\ResourceManager.h"

namespace HBL2
{
	class SceneManager
	{
	public:
		SceneManager(const SceneManager&) = delete;

		static SceneManager& Get()
		{
			static SceneManager instance;
			return instance;
		}

		Handle<Scene> LoadScene(Handle<Asset> sceneAssetHandle, bool runtime = false);

	private:
		SceneManager() = default;
	};
}