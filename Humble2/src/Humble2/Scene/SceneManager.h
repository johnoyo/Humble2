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
	namespace Editor
	{
		class EditorPanelSystem;
	}

	class HBL2_API SceneManager
	{
	public:
		SceneManager(const SceneManager&) = delete;

		static SceneManager& Get();

		void LoadScene(Handle<Asset> sceneAssetHandle) { LoadScene(sceneAssetHandle, true); }
		void LoadScene(Handle<Scene> sceneHandle) { LoadScene(sceneHandle, true); }

	private:
		void LoadPlaymodeScene(Handle<Scene> sceneHandle, bool runtime);
		void LoadScene(Handle<Asset> sceneAssetHandle, bool runtime);
		void LoadScene(Handle<Scene> sceneHandle, bool runtime);
		void LoadSceneDeffered();

		void LoadSceneFromAsset();
		void LoadSceneFromResource();
		void LoadSceneFromResourceForPlaymode();

		bool SceneChangeRequested = false;

	private:
		SceneManager() = default;

		Handle<Asset> m_NewSceneAssetHandle;
		Handle<Scene> m_NewSceneHandle;

		Handle<Scene> m_CurrentSceneHandle;
		Handle<Asset> m_CurrentSceneAssetHandle;

		enum class SceneChangeSource
		{
			None = 0,
			Asset,
			Resource,
			ResourcePlayMode,
		};

		bool m_RuntimeSceneChange = false;
		SceneChangeSource m_SceneChangeSource = SceneChangeSource::None;

		friend class Project;
		friend class Application;
		friend class Editor::EditorPanelSystem;
	};
}