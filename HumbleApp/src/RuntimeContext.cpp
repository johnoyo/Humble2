#include "RuntimeContext.h"

namespace HBL2Runtime
{
    void RuntimeContext::OnCreate()
    {
		HBL2::Context::Mode = HBL2::Mode::Runtime;

		m_ActiveScene = HBL2::ResourceManager::Instance->GetScene(ActiveScene);

		OpenProject();

		for (HBL2::ISystem* system : m_ActiveScene->GetSystems())
		{
			system->OnCreate();
		}
    }

    void RuntimeContext::OnUpdate(float ts)
    {
		for (HBL2::ISystem* system : m_ActiveScene->GetSystems())
		{
			system->OnUpdate(ts);
		}
    }

    void RuntimeContext::OnGuiRender(float ts)
    {
		for (HBL2::ISystem* system : m_ActiveScene->GetSystems())
		{
			system->OnGuiRender(ts);
		}
    }

	void RuntimeContext::OnDestroy()
	{
		for (HBL2::ISystem* system : m_ActiveScene->GetSystems())
		{
			system->OnDestroy();
		}
	}

	bool RuntimeContext::OpenProject()
	{
		std::filesystem::path filepath;

		for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::current_path()))
		{
			if (entry.path().extension() == ".hblproj")
			{
				filepath = entry.path();
			}
		}

		if (filepath.empty())
		{
			HBL2_ERROR("Could not find project.");
			return false;
		}

		if (HBL2::Project::Load(std::filesystem::path(filepath)) != nullptr)
		{
			HBL2::Project::OpenStartingScene();
			return true;
		}

		HBL2_ERROR("Could not open specified project at path \"{0}\".", filepath);
		HBL2::Window::Instance->Close();

		return false;
	}
}