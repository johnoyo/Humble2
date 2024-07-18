#include "EditorContext.h"
#include <Utilities\FileDialogs.h>

namespace HBL2
{
	namespace Editor
	{
		void EditorContext::OnAttach()
		{
			Mode = HBL2::Mode::Editor;

			// Create FrameBuffer.
			HBL2::Renderer::Instance->FrameBufferHandle = HBL2::ResourceManager::Instance->CreateFrameBuffer({
				.debugName = "editor-viewport",
				.width = 1280,
				.height = 720,
			});

			//if (!OpenEmptyProject())
			//{
				ActiveScene = EmptyScene;
			//	return;
			//}

			// Create editor systems.
			Core->RegisterSystem(new EditorPanelSystem);
			Core->RegisterSystem(new EditorCameraSystem);

			// Editor camera set up.
			auto editorCameraEntity = Core->CreateEntity();
			Core->GetComponent<HBL2::Component::Tag>(editorCameraEntity).Name = "Hidden";
			Core->AddComponent<Component::EditorCamera>(editorCameraEntity);
			Core->AddComponent<HBL2::Component::Camera>(editorCameraEntity).Enabled = true;
			Core->GetComponent<HBL2::Component::Transform>(editorCameraEntity).Translation.z = 10.f;

			auto entity = ActiveScene->CreateEntity();
			ActiveScene->GetComponent<HBL2::Component::Tag>(entity).Name = "Sprite";
			ActiveScene->GetComponent<HBL2::Component::Transform>(entity).Translation = { 0.f, 0.f, 0.f };
			ActiveScene->AddComponent<HBL2::Component::EditorVisible>(entity);
			ActiveScene->AddComponent<HBL2::Component::StaticMesh_New>(entity);
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
				const auto& startingScenePath = HBL2::Project::GetAssetFileSystemPath(HBL2::Project::GetActive()->GetSpecification().StartingScene);

				HBL2::Project::OpenScene(startingScenePath);

				return true;
			}

			HBL2_ERROR("Could not open specified project at path \"{0}\".", filepath);
			HBL2::Application::Get().GetWindow()->Close();

			return false;
		}
	}
}