#include "RuntimeContext.h"

namespace HBL2
{
	namespace Runtime
	{
		void RuntimeContext::OnAttach()
		{
			HBL2::Context::Mode = HBL2::Mode::Runtime;
			HBL2::AssetManager::Instance = new HBL2::EditorAssetManager;

			OpenProject();

			HBL2::EventDispatcher::Get().Register<SceneChangeEvent>([&](const HBL2::SceneChangeEvent& e)
			{
				m_ActiveScene = HBL2::ResourceManager::Instance->GetScene(e.NewScene);
			});

			Context::ViewportPosition = { 0, 0 };
			Context::ViewportSize = Window::Instance->GetExtents();

			HBL2::EventDispatcher::Get().Register<WindowSizeEvent>([&](const HBL2::WindowSizeEvent& e)
			{
				Context::ViewportSize = { e.Width, e.Height };

				if (e.Width == 0 || e.Height == 0)
				{
					return;
				}

				m_ActiveScene->GetRegistry()
					.view<HBL2::Component::Camera>()
					.each([&](HBL2::Component::Camera& camera)
					{
						if (camera.Enabled)
						{
							camera.AspectRatio = Context::ViewportSize.x / Context::ViewportSize.y;
						}
					});
			});

			ImGui::SetCurrentContext(HBL2::ImGuiRenderer::Instance->GetContext());

			// NOTE: The OnAttach method of the registered systems will be called from the SceneManager class.
		}

		void RuntimeContext::OnCreate()
		{
			// NOTE: The OnCreate method of the registered systems will be called from the SceneManager class.
		}

		void RuntimeContext::OnUpdate(float ts)
		{
			if (m_ActiveScene == nullptr)
			{
				return;
			}

			for (HBL2::ISystem* system : m_ActiveScene->GetSystems())
			{
				if (system->GetState() == HBL2::SystemState::Play)
				{
					system->OnUpdate(ts);
				}
			}
		}

		void RuntimeContext::OnFixedUpdate()
		{
			m_AccumulatedTime += Time::DeltaTime;

			while (m_AccumulatedTime >= Time::FixedTimeStep)
			{
				if (m_ActiveScene == nullptr)
				{
					return;
				}

				for (HBL2::ISystem* system : m_ActiveScene->GetSystems())
				{
					if (system->GetState() == HBL2::SystemState::Play)
					{
						system->OnFixedUpdate();
					}
				}

				m_AccumulatedTime -= Time::FixedTimeStep;
			}
		}

		void RuntimeContext::OnGuiRender(float ts)
		{
			if (m_ActiveScene == nullptr)
			{
				return;
			}

			for (HBL2::ISystem* system : m_ActiveScene->GetSystems())
			{
				if (system->GetState() == HBL2::SystemState::Play)
				{
					system->OnGuiRender(ts);
				}
			}
		}

		void RuntimeContext::OnDestroy()
		{
			if (m_ActiveScene == nullptr)
			{
				return;
			}

			for (HBL2::ISystem* system : m_ActiveScene->GetSystems())
			{
				system->OnDestroy();
			}

			ImGui::SetCurrentContext(nullptr);

			TextureUtilities::Get().DeleteWhiteTexture();
			ShaderUtilities::Get().DeleteBuiltInShaders();
			ShaderUtilities::Get().DeleteBuiltInMaterials();
			MeshUtilities::Get().DeleteBuiltInMeshes();
		}

		void RuntimeContext::OnDetach()
		{
			if (m_ActiveScene == nullptr)
			{
				return;
			}

			for (HBL2::ISystem* system : m_ActiveScene->GetSystems())
			{
				system->OnDetach();
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
				TextureUtilities::Get().LoadWhiteTexture();
				ShaderUtilities::Get().LoadBuiltInShaders();
				ShaderUtilities::Get().LoadBuiltInMaterials();
				MeshUtilities::Get().LoadBuiltInMeshes();

				HBL2::Project::OpenStartingScene(true);
				return true;
			}

			HBL2_ERROR("Could not open specified project at path \"{0}\".", filepath);
			HBL2::Window::Instance->Close();

			return false;
		}
	}
}