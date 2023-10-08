#include "RuntimeContext.h"

namespace HBL2Runtime
{
    void RuntimeContext::OnAttach()
    {
		HBL2::Context::Mode = HBL2::Mode::Runtime;
#ifdef EMSCRIPTEN
		// Runtime camera setup.
		auto camera = ActiveScene->CreateEntity();
		ActiveScene->GetComponent<HBL2::Component::Tag>(camera).Name = "Camera";
		ActiveScene->AddComponent<HBL2::Component::Camera>(camera).Enabled = true;
		ActiveScene->GetComponent<HBL2::Component::Transform>(camera).Translation.z = 100.f;

		// Add a monkeh.
		auto monkeh = ActiveScene->CreateEntity();
		ActiveScene->GetComponent<HBL2::Component::Tag>(monkeh).Name = "Monkeh";
		ActiveScene->GetComponent<HBL2::Component::Transform>(monkeh).Scale = { 5.f, 5.f, 5.f };
		auto& mesh = ActiveScene->AddComponent<HBL2::Component::StaticMesh>(monkeh);
		mesh.Path = "assets/meshes/monkey_smooth.obj";
		mesh.ShaderName = "BasicMesh";
#else
		OpenProject();
#endif
    }

    void RuntimeContext::OnCreate()
    {
		for (HBL2::ISystem* system : Core->GetSystems())
		{
			system->OnCreate();
		}

		for (HBL2::ISystem* system : ActiveScene->GetSystems())
		{
			system->OnCreate();
		}
    }

    void RuntimeContext::OnUpdate(float ts)
    {
		for (HBL2::ISystem* system : Core->GetSystems())
		{
			system->OnUpdate(ts);
		}

		for (HBL2::ISystem* system : ActiveScene->GetSystems())
		{
			system->OnUpdate(ts);
		}
    }

    void RuntimeContext::OnGuiRender(float ts)
    {
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
			// HBL2_ERROR("Could not find project.");
		}

		if (HBL2::Project::Load(std::filesystem::path(filepath)) != nullptr)
		{
			auto& startingScenePath = HBL2::Project::GetAssetFileSystemPath(HBL2::Project::GetActive()->GetSpecification().StartingScene);

			HBL2::Project::OpenScene(startingScenePath);

			return true;
		}

		// HBL2_ERROR("Could not open specified project at path \"{0}\".", filepath);
		HBL2::Application::Get().GetWindow()->Close();

		return false;
	}
}