#include "Project.h"

#include "ProjectSerializer.h"
#include <Core\Context.h>

#include "Systems\TransformSystem.h"
#include "Systems\LinkSystem.h"
#include "Systems\CameraSystem.h"
#include "Systems\StaticMeshRenderingSystem.h"

namespace HBL2
{
	Project* Project::Create(const std::string& name)
	{
		s_ActiveProject = new Project;

		s_ActiveProject->m_Spec.StartingScene = "Scenes/EmptyScene.humble";
		s_ActiveProject->m_Spec.AssetDirectory = "Assets";
		s_ActiveProject->m_Spec.Name = name;		

		return s_ActiveProject;
	}

	Project* Project::Load(const std::filesystem::path& path)
	{
		if (path.empty())
		{
			return nullptr;
		}

		Project* project = new Project;

		ProjectSerializer serializer(project);

		if (serializer.Deserialize(path))
		{
			delete s_ActiveProject;

			s_ActiveProject = project;
			s_ActiveProject->m_ProjectDirectory = path.parent_path();

			return s_ActiveProject;
		}

		return nullptr;
	}

	void Project::Save(const std::filesystem::path& path)
	{
		s_ActiveProject->m_ProjectDirectory = path.parent_path();

		ProjectSerializer serializer(s_ActiveProject);
		serializer.Serialize(path);
	}

	void Project::OpenStartingScene()
	{
		UUID assetUUID = std::hash<std::string>()(s_ActiveProject->GetSpecification().StartingScene.string());
		Handle<Scene> sceneHandle = AssetManager::Instance->GetAsset<Scene>(assetUUID);
		// Context::ActiveScene = newScene;
	}

	void Project::OpenScene(const std::filesystem::path& path)
	{
		if (path.extension().string() != ".humble")
		{
			HBL2_WARN("Could not load {0} - not a scene file", path.filename().string());
			return;
		}

		Scene* newScene = new Scene({ .name = s_ActiveProject->GetSpecification().StartingScene.string() });

		SceneSerializer serializer(newScene);
		if (serializer.Deserialize(path.string()))
		{
			delete Context::ActiveScene;
			Context::ActiveScene = newScene;

			Context::ActiveScene->RegisterSystem(new TransformSystem);
			Context::ActiveScene->RegisterSystem(new LinkSystem);
			Context::ActiveScene->RegisterSystem(new CameraSystem);
			Context::ActiveScene->RegisterSystem(new StaticMeshRenderingSystem);
		}
	}

	void Project::SaveScene(Scene* scene, const std::filesystem::path& path)
	{
		HBL2::SceneSerializer serializer(scene);
		serializer.Serialize(path.string());
	}
}