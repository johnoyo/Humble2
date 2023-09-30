#include "Project.h"

#include "ProjectSerializer.h"
#include <Core\Context.h>

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

	Project* Project::Load(std::filesystem::path& path)
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

	void Project::OpenScene(const std::filesystem::path& path)
	{
		if (path.extension().string() != ".humble")
		{
			HBL2_WARN("Could not load {0} - not a scene file", path.filename().string());
			return;
		}

		Scene* newScene = new Scene(s_ActiveProject->GetSpecification().StartingScene.string());

		SceneSerializer serializer(newScene);
		if (serializer.Deserialize(path.string()))
		{
			delete Context::ActiveScene;
			Context::ActiveScene = newScene;
		}
	}

	void Project::SaveScene(Scene* scene, const std::filesystem::path& path)
	{
		HBL2::SceneSerializer serializer(scene);
		serializer.Serialize(path.string());
	}
}