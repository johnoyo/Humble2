#include "EditorContext.h"
#include <Utilities\FileDialogs.h>

namespace HBL2
{
	namespace Editor
	{
		void EditorContext::OnCreate()
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

			// Create editor systems.
			for (HBL2::ISystem* system : m_EditorScene->GetSystems())
			{
				system->OnCreate();
			}

			EventDispatcher::Get().Register("SceneChangeEvent", [&](const HBL2::Event& e)
			{
				HBL2_CORE_INFO("EditorContext::SceneChangeEvent");
				const SceneChangeEvent& sce = dynamic_cast<const SceneChangeEvent&>(e);
				m_ActiveScene = ResourceManager::Instance->GetScene(sce.NewScene);
			});

			ImGui::SetCurrentContext(HBL2::ImGuiRenderer::Instance->GetContext());
		}

		void EditorContext::OnUpdate(float ts)
		{
			for (HBL2::ISystem* system : m_EditorScene->GetSystems())
			{
				system->OnUpdate(ts);
			}

			if (m_ActiveScene == nullptr)
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

		void EditorContext::OnGuiRender(float ts)
		{
			ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

			for (HBL2::ISystem* system : m_EditorScene->GetSystems())
			{
				system->OnGuiRender(ts);
			}

			if (m_ActiveScene == nullptr)
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

			if (m_ActiveScene != nullptr)
			{
				for (HBL2::ISystem* system : m_ActiveScene->GetSystems())
				{
					system->OnDestroy();
				}
			}

			ImGui::SetCurrentContext(nullptr);
		}

		bool EditorContext::OpenEmptyProject()
		{
			std::string filepath = HBL2::FileDialogs::OpenFile("Humble Project (*.hblproj)\0*.hblproj\0");

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
			UnityBuilder::Get().LoadUnityBuildScript();
		}
	}
}