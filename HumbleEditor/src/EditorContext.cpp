#include "EditorContext.h"
#include <Utilities\FileDialogs.h>

namespace HBL2
{
	namespace Editor
	{
		void EditorContext::OnAttach()
		{
			Mode = HBL2::Mode::Editor;
			AssetManager::Instance = new EditorAssetManager;

			if (!OpenEmptyProject())
			{
				ActiveScene = EmptyScene;
				return;
			}

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

			EventDispatcher::Get().Register<SceneChangeEvent>([&](const HBL2::SceneChangeEvent& e)
			{
				HBL2_CORE_INFO("EditorContext::SceneChangeEvent");
				m_ActiveScene = ResourceManager::Instance->GetScene(e.NewScene);
			});

			ImGui::SetCurrentContext(HBL2::ImGuiRenderer::Instance->GetContext());

			// NOTE: The OnCreate method of the registered systems will be called from the SceneManager class.
		}

		void EditorContext::OnCreate()
		{
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
			ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

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

			TextureUtilities::Get().DeleteWhiteTexture();
			ShaderUtilities::Get().DeleteBuiltInShaders();
			ShaderUtilities::Get().DeleteBuiltInMaterials();
			MeshUtilities::Get().DeleteBuiltInMeshes();
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

		bool EditorContext::OpenEmptyProject()
		{
			std::string filepath = HBL2::FileDialogs::OpenFile("Humble Project", "", {"Humble Project Files (*.hblproj)", "*.hblproj"});

			if (HBL2::Project::Load(std::filesystem::path(filepath)) != nullptr)
			{
				LoadBuiltInAssets();

				HBL2::Project::OpenStartingScene();

				return true;
			}

			HBL2_ERROR("Could not open specified project at path \"{0}\".", filepath);
			HBL2::Window::Instance->Close();

			return false;
		}

		void EditorContext::LoadBuiltInAssets()
		{
			TextureUtilities::Get().LoadWhiteTexture();
			ShaderUtilities::Get().LoadBuiltInShaders();
			ShaderUtilities::Get().LoadBuiltInMaterials();
			MeshUtilities::Get().LoadBuiltInMeshes();
		}

		bool EditorContext::IsActiveSceneValid()
		{
			Scene* activeScene = ResourceManager::Instance->GetScene(ActiveScene);
			return m_ActiveScene != nullptr && activeScene != nullptr;
		}
	}
}