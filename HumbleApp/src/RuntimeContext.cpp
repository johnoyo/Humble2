#include "RuntimeContext.h"

namespace HBL2
{
	namespace Runtime
	{
		void RuntimeContext::OnAttach()
		{
			HBL2::Context::Mode = HBL2::Mode::Runtime;
			OpenProject();
		}

		void RuntimeContext::OnCreate()
		{
			LoadProject();

			HBL2::EventDispatcher::Get().Register<SceneChangeEvent>([&](const HBL2::SceneChangeEvent& e)
			{
				m_ActiveScene = HBL2::ResourceManager::Instance->GetScene(e.NewScene);
			});
			
			Context::ViewportPosition = Window::Instance->GetPosition();
			Context::ViewportSize = Window::Instance->GetExtents();

			HBL2::EventDispatcher::Get().Register<HBL2::WindowPositionEvent>([&](const HBL2::WindowPositionEvent& e)
			{
				Context::ViewportPosition = { e.XPosition, e.YPosition };
			});

			HBL2::EventDispatcher::Get().Register<HBL2::WindowSizeEvent>([&](const HBL2::WindowSizeEvent& e)
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

			// NOTE: The OnAttach and OnCreate method of the registered systems will be called from the SceneManager class.

			ImGui::SetCurrentContext(HBL2::ImGuiRenderer::Instance->GetContext());
		}

		void RuntimeContext::OnUpdate(float ts)
		{
			if (m_ActiveScene == nullptr)
			{
				return;
			}

			if (m_FirstFrame)
			{
				m_ActiveScene->GetRegistry()
					.view<HBL2::Component::Camera>()
					.each([&](HBL2::Component::Camera& camera)
					{
						if (camera.Enabled)
						{
							camera.AspectRatio = Context::ViewportSize.x / Context::ViewportSize.y;
						}
					});

				m_FirstFrame = false;
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
				return true;
			}

			HBL2_ERROR("Could not open specified project at path \"{0}\".", filepath);

			return false;
		}

		void RuntimeContext::LoadProject()
		{
			HBL2::Project::OpenStartingScene(true);

			Project::ApplySettings();
		}
	}
}