#include "Project.h"

#include "ProjectSerializer.h"
#include <Core\Context.h>

namespace HBL2
{
	Project* Project::Create(const std::string& name)
	{
		s_ActiveProject = new Project;

		s_ActiveProject->m_Spec.StartingScene = std::filesystem::path("Scenes") / std::filesystem::path("EmptyScene.humble");
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
		AssetManager::Instance->RegisterAssets();

		UUID assetUUID = std::hash<std::string>()(s_ActiveProject->GetSpecification().StartingScene.string());
		Handle<Scene> sceneHandle = AssetManager::Instance->GetAsset<Scene>(assetUUID);

		if (!sceneHandle.IsValid())
		{
			HBL2_CORE_ERROR("Scene asset handle of \"{0}\" is invalid, aborting scene load.", s_ActiveProject->GetSpecification().StartingScene.string());
			return;
		}

		const std::filesystem::path& startingScenePath = GetAssetFileSystemPath(s_ActiveProject->GetSpecification().StartingScene.string());

		if (!std::filesystem::is_directory(startingScenePath.parent_path()))
		{
			Scene* scene = ResourceManager::Instance->GetScene(sceneHandle);
			HBL2::SceneSerializer serializer(scene);
			serializer.Serialize(GetAssetFileSystemPath(s_ActiveProject->GetSpecification().StartingScene.string()));
		}

		SceneManager::Get().LoadScene(sceneHandle, true);
	}
}