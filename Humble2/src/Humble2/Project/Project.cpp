#include "Project.h"

#include "ProjectSerializer.h"
#include <Core\Context.h>

namespace HBL2
{
	Project* Project::s_ActiveProject = nullptr;

	Project* Project::Create(const std::string& name)
	{
		s_ActiveProject = new Project;

		s_ActiveProject->m_Spec.StartingScene = std::filesystem::path("Scenes") / std::filesystem::path("EmptyScene.humble");
		s_ActiveProject->m_Spec.AssetDirectory = "Assets";
		s_ActiveProject->m_Spec.ScriptDirectory = std::filesystem::path("Assets") / std::filesystem::path("Scripts");
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

	void Project::OpenStartingScene(bool runtime)
	{
		AssetManager::Instance->RegisterAssets();

		const auto& startingSceneMetadataPath = Project::GetAssetFileSystemPath(s_ActiveProject->GetSpecification().StartingScene).string() + ".hblscene";

		// Open metadata file to retrieve asset UUID.
		std::ifstream stream(startingSceneMetadataPath);

		if (!stream.is_open())
		{
			HBL2_CORE_ERROR("Scene metadata file of starting scene not found: {0}", startingSceneMetadataPath);
			return;
		}

		std::stringstream ss;
		ss << stream.rdbuf();

		YAML::Node dataMetadata = YAML::Load(ss.str());
		if (!dataMetadata["Scene"].IsDefined())
		{
			HBL2_CORE_TRACE("Scene not found in metadata file: {0}, not a valid file format.", ss.str());
			stream.close();
			return;
		}

		UUID assetUUID = 0;

		auto sceneMetadataProperties = dataMetadata["Scene"];
		if (sceneMetadataProperties)
		{
			assetUUID = sceneMetadataProperties["UUID"].as<UUID>();
		}

		stream.close();

		// Load scene asset.
		Handle<Scene> sceneHandle = AssetManager::Instance->GetAsset<Scene>(assetUUID);

		if (!sceneHandle.IsValid())
		{
			HBL2_CORE_ERROR("Scene asset handle of \"{0}\" is invalid, aborting scene load.", s_ActiveProject->GetSpecification().StartingScene.string());
			return;
		}

		const std::filesystem::path& startingScenePath = GetAssetFileSystemPath(s_ActiveProject->GetSpecification().StartingScene.string());

		// If it is a newly created project and the starting scene file is not created, create them.
		if (!std::filesystem::is_directory(startingScenePath.parent_path()))
		{
			Scene* scene = ResourceManager::Instance->GetScene(sceneHandle);
			HBL2::SceneSerializer serializer(scene);
			serializer.Serialize(GetAssetFileSystemPath(s_ActiveProject->GetSpecification().StartingScene.string()));
		}

		SceneManager::Get().LoadScene(sceneHandle, runtime);
	}
}