#include "EditorContext.h"
#include <Utilities\FileDialogs.h>

namespace HBL2Editor
{
	void EditorContext::OnAttach()
	{
		Mode = HBL2::Mode::Editor;

		// Create FrameBuffer.
		HBL2::FrameBufferSpecification spec;
		spec.Width = 1280;
		spec.Height = 720;
		HBL2::RenderCommand::FrameBuffer = HBL2::FrameBuffer::Create(spec);

		if (!OpenEmptyProject())
		{
			ActiveScene = EmptyScene;
			return;
		}

		// Create editor systems.
		Core->RegisterSystem(new EditorPanelSystem);
		Core->RegisterSystem(new EditorCameraSystem);

		// Editor camera set up.
		auto editorCameraEntity = Core->CreateEntity();
		Core->GetComponent<HBL2::Component::Tag>(editorCameraEntity).Name = "Hidden";
		Core->AddComponent<Component::EditorCamera>(editorCameraEntity);
		Core->AddComponent<HBL2::Component::Camera>(editorCameraEntity).Enabled = true;
		Core->GetComponent<HBL2::Component::Transform>(editorCameraEntity).Translation.z = 100.f;
	}

	void EditorContext::OnCreate()
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

	void EditorContext::OnUpdate(float ts)
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

	void EditorContext::OnGuiRender(float ts)
	{
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

		for (HBL2::ISystem* system : Core->GetSystems())
		{
			system->OnGuiRender(ts);
		}

		for (HBL2::ISystem* system : ActiveScene->GetSystems())
		{
			system->OnGuiRender(ts);
		}
	}

	bool EditorContext::OpenEmptyProject()
	{
		std::string filepath = HBL2::FileDialogs::OpenFile("Humble Project (*.hblproj)\0*.hblproj\0");

		if (HBL2::Project::Load(std::filesystem::path(filepath)) != nullptr)
		{
			auto& startingScenePath = HBL2::Project::GetAssetFileSystemPath(HBL2::Project::GetActive()->GetSpecification().StartingScene);

			HBL2::Project::OpenScene(startingScenePath);

			return true;
		}

		HBL2_ERROR("Could not open specified project at path \"{0}\".", filepath);
		HBL2::Application::Get().GetWindow()->Close();

		return false;
	}
}