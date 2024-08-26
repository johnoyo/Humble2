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
			ActiveScene->RegisterSystem(new EditorCameraSystem);

			// Editor camera set up.
			auto editorCameraEntity = ActiveScene->CreateEntity();
			ActiveScene->GetComponent<HBL2::Component::Tag>(editorCameraEntity).Name = "Hidden";
			ActiveScene->AddComponent<Component::EditorCamera>(editorCameraEntity);
			ActiveScene->AddComponent<HBL2::Component::Camera>(editorCameraEntity).Enabled = true;
			ActiveScene->GetComponent<HBL2::Component::Transform>(editorCameraEntity).Translation.z = 5.f;

			auto entity1 = ActiveScene->CreateEntity();
			ActiveScene->GetComponent<HBL2::Component::Tag>(entity1).Name = "Sprite1";
			ActiveScene->GetComponent<HBL2::Component::Transform>(entity1).Translation = { -1.f, 0.5f, 0.f };
			ActiveScene->AddComponent<HBL2::Component::EditorVisible>(entity1);
			ActiveScene->AddComponent<HBL2::Component::StaticMesh_New>(entity1);

			auto entity2 = ActiveScene->CreateEntity();
			ActiveScene->GetComponent<HBL2::Component::Tag>(entity2).Name = "Sprite2";
			ActiveScene->GetComponent<HBL2::Component::Transform>(entity2).Translation = { 1.f, 0.5f, 0.f };
			ActiveScene->AddComponent<HBL2::Component::EditorVisible>(entity2);
			ActiveScene->AddComponent<HBL2::Component::StaticMesh_New>(entity2);

			auto entity3 = ActiveScene->CreateEntity();
			ActiveScene->GetComponent<HBL2::Component::Tag>(entity3).Name = "Sprite3";
			ActiveScene->GetComponent<HBL2::Component::Transform>(entity3).Translation = { 0.f, 0.f, 0.f };
			ActiveScene->AddComponent<HBL2::Component::EditorVisible>(entity3);
			ActiveScene->AddComponent<HBL2::Component::StaticMesh_New>(entity3);

			uint32_t entityCount = 1000;
			std::vector<entt::entity> entities;
			entities.resize(entityCount);

			for (int i = 0; i < entityCount; i++)
			{
				entities[i] = ActiveScene->CreateEntity();
				ActiveScene->GetComponent<HBL2::Component::Tag>(entities[i]).Name = "entity" + std::to_string(i);
				ActiveScene->GetComponent<HBL2::Component::Transform>(entities[i]).Translation = { 0.f, 0.f, 0.f };
				ActiveScene->AddComponent<HBL2::Component::EditorVisible>(entities[i]);
				ActiveScene->AddComponent<HBL2::Component::StaticMesh_New>(entities[i]);
			}
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
			HBL2::Window::Instance->Close();

			return false;
		}
	}
}