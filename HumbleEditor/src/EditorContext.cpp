#include "EditorContext.h"

#include <Utilities\FileDialogs.h>

namespace HBL2
{
	namespace Editor
	{
		void EditorContext::OnAttach()
		{
			Mode = HBL2::Mode::Editor;

			if (!OpenProject())
			{
				ActiveScene = EmptyScene;
				return;
			}
		}

		void EditorContext::OnCreate()
		{
			LoadProject();

			m_EditorScene = ResourceManager::Instance->GetScene(EditorScene);
			m_EmptyScene = ResourceManager::Instance->GetScene(EmptyScene);

			// Create editor systems.
			m_EditorScene->RegisterSystem(new EditorPanelSystem);
			m_EditorScene->RegisterSystem(new TransformSystem);
			m_EditorScene->RegisterSystem(new CameraSystem);
			m_EditorScene->RegisterSystem(new EditorCameraSystem);

			// Editor camera set up.
			auto editorCameraEntity = m_EditorScene->CreateEntity("Hidden");
			m_EditorScene->AddComponent<HBL2::Component::Camera>(editorCameraEntity).Enabled = true;
			m_EditorScene->AddComponent<Component::EditorCamera>(editorCameraEntity);
			m_EditorScene->GetComponent<HBL2::Component::Transform>(editorCameraEntity).Translation.y = 0.5f;
			m_EditorScene->GetComponent<HBL2::Component::Transform>(editorCameraEntity).Translation.z = 5.f;

			for (HBL2::ISystem* system : m_EditorScene->GetSystems())
			{
				system->OnAttach();
			}
			// NOTE: The OnAttach method of the registered systems will be called from the SceneManager class.

			EventDispatcher::Get().Register<SceneChangeEvent>([&](const HBL2::SceneChangeEvent& e)
			{
				HBL2_CORE_INFO("EditorContext::SceneChangeEvent");
				m_ActiveScene = ResourceManager::Instance->GetScene(e.NewScene);
			});

			ImGui::SetCurrentContext(HBL2::ImGuiRenderer::Instance->GetContext());

			if (m_EditorScene == nullptr)
			{
				return;
			}

			for (HBL2::ISystem* system : m_EditorScene->GetSystems())
			{
				system->OnCreate();
			}
			// NOTE: The OnCreate method of the registered systems will be called from the SceneManager class.
		}

		void EditorContext::OnUpdate(float ts)
		{
			for (HBL2::ISystem* system : m_EditorScene->GetSystems())
			{
				system->OnUpdate(ts);
			}

			if (!IsActiveSceneValid())
			{
				HBL2_CORE_TRACE("Active Scene is null.");
				return;
			}

			for (HBL2::ISystem* system : m_ActiveScene->GetCoreSystems())
			{
				if (system->GetState() == SystemState::Play)
				{
					system->OnUpdate(ts);
				}
			}

			if (Mode == Mode::Runtime)
			{
				for (HBL2::ISystem* system : m_ActiveScene->GetRuntimeSystems())
				{
					if (system->GetState() == SystemState::Play)
					{
						system->OnUpdate(ts);
					}
				}
			}
		}

		void EditorContext::OnFixedUpdate()
		{
			m_AccumulatedTime += Time::DeltaTime;

			while (m_AccumulatedTime >= Time::FixedTimeStep)
			{
				for (HBL2::ISystem* system : m_EditorScene->GetSystems())
				{
					system->OnFixedUpdate();
				}

				if (!IsActiveSceneValid())
				{
					HBL2_CORE_TRACE("Active Scene is null.");
					return;
				}

				for (HBL2::ISystem* system : m_ActiveScene->GetCoreSystems())
				{
					if (system->GetState() == SystemState::Play)
					{
						system->OnFixedUpdate();
					}
				}

				if (Mode == Mode::Runtime)
				{
					for (HBL2::ISystem* system : m_ActiveScene->GetRuntimeSystems())
					{
						if (system->GetState() == SystemState::Play)
						{
							system->OnFixedUpdate();
						}
					}
				}

				m_AccumulatedTime -= Time::FixedTimeStep;
			}
		}

		void EditorContext::OnGuiRender(float ts)
		{
			ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

			for (HBL2::ISystem* system : m_EditorScene->GetSystems())
			{
				system->OnGuiRender(ts);
			}

			if (!IsActiveSceneValid())
			{
				HBL2_CORE_TRACE("Active Scene is null.");
				return;
			}

			for (HBL2::ISystem* system : m_ActiveScene->GetCoreSystems())
			{
				if (system->GetState() == SystemState::Play)
				{
					system->OnGuiRender(ts);
				}
			}

			if (Mode == Mode::Runtime)
			{
				for (HBL2::ISystem* system : m_ActiveScene->GetRuntimeSystems())
				{
					if (system->GetState() == SystemState::Play)
					{
						system->OnGuiRender(ts);
					}
				}
			}
		}

		void EditorContext::OnGizmoRender(float ts)
		{
			for (HBL2::ISystem* system : m_EditorScene->GetSystems())
			{
				system->OnGizmoRender(ts);
			}

			if (!IsActiveSceneValid())
			{
				HBL2_CORE_TRACE("Active Scene is null.");
				return;
			}

			for (HBL2::ISystem* system : m_ActiveScene->GetCoreSystems())
			{
				if (system->GetState() == SystemState::Play)
				{
					system->OnGizmoRender(ts);
				}
			}

			if (Mode == Mode::Runtime)
			{
				for (HBL2::ISystem* system : m_ActiveScene->GetRuntimeSystems())
				{
					if (system->GetState() == SystemState::Play)
					{
						system->OnGizmoRender(ts);
					}
				}
			}
		}

		void EditorContext::OnDestroy()
		{
			if (m_EditorScene != nullptr)
			{
				for (HBL2::ISystem* system : m_EditorScene->GetSystems())
				{
					system->OnDestroy();
				}
			}

			if (IsActiveSceneValid())
			{
				for (HBL2::ISystem* system : m_ActiveScene->GetSystems())
				{
					system->OnDestroy();
				}
			}

			ImGui::SetCurrentContext(nullptr);
		}

		void EditorContext::OnDetach()
		{
			if (m_EditorScene != nullptr)
			{
				for (HBL2::ISystem* system : m_EditorScene->GetSystems())
				{
					system->OnDetach();
				}
			}

			if (IsActiveSceneValid())
			{
				for (HBL2::ISystem* system : m_ActiveScene->GetSystems())
				{
					system->OnDetach();
				}
			}
		}

		bool EditorContext::OpenProject()
		{
			std::string filepath = HBL2::FileDialogs::OpenFile("Humble Project", "", {"Humble Project Files (*.hblproj)", "*.hblproj"});

			if (HBL2::Project::Load(std::filesystem::path(filepath)) != nullptr)
			{
				return true;
			}

			HBL2_ERROR("Could not open specified project at path \"{0}\".", filepath);

			return false;
		}

		void EditorContext::LoadProject()
		{
			HBL2::Project::OpenStartingScene();

			Project::ApplySettings();
		}

		bool EditorContext::IsActiveSceneValid()
		{
			Scene* activeScene = ResourceManager::Instance->GetScene(ActiveScene);
			return m_ActiveScene != nullptr && activeScene != nullptr;
		}
	}
}