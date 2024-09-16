#pragma once

#include "Core\Context.h"
#include "Core\Events.h"
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

		void LoadScene(Handle<Asset> sceneAssetHandle, bool runtime = false);
		void LoadScene(Handle<Scene> sceneHandle, bool runtime = false);

		bool SceneChangeRequested = false;

	private:
		SceneManager() = default;
		void LoadSceneDeffered();

	private:
		Handle<Asset> m_NewSceneAssetHandle;
		Handle<Scene> m_NewSceneHandle;
		bool m_RuntimeSceneChange = false;

		friend class Application;
	};
}