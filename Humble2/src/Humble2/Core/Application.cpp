#include "Application.h"

namespace HBL2
{
	Application::Application(ApplicationSpec& specification) : m_Specification(specification)
	{
		Log::Initialize();

		switch (m_Specification.Platform)
		{
		case Platform::Windows:
			HBL2_CORE_INFO("Windows platform selected.");
			break;
		case Platform::Web:
			HBL2_CORE_INFO("Web platform selected.");
			break;
		case Platform::None:
			HBL2_CORE_ERROR("No target platform specified. Please choose between Windows or Web.");
			exit(-1);
			break;
		default:
			break;
		}

		switch (m_Specification.GraphicsAPI)
		{
		case GraphicsAPI::OpenGL:
			HBL2_CORE_INFO("OpenGL is selected as the renderer API.");
			break;
		case GraphicsAPI::Vulkan:
			if (m_Specification.Platform == Platform::Web)
			{
				HBL2_CORE_WARN("Web is the selected platform, defaulting to OpenGLES as the renderer API.");
				m_Specification.GraphicsAPI = GraphicsAPI::OpenGL;
				break;
			}
			HBL2_CORE_INFO("Vulkan is selected as the renderer API.");
			break;
		case GraphicsAPI::None:
			HBL2_CORE_ERROR("No RendererAPI specified. Please choose between OpenGL, or Vulkan depending on your target platform.");
			exit(-1);
			break;
		default:
			break;
		}

		// Intialize the window.
		m_Window = new Window(m_Specification.Name, m_Specification.Width, m_Specification.Height, m_Specification.Fullscreen, m_Specification.Vsync);
	}

	Application::~Application()
	{
		delete m_Window;
	}

	void Application::BeginFrame()
	{
		// Measure time and delta time.
		float time = (float)m_Window->GetTime();
		m_DeltaTime = time - m_LastTime;
		m_FixedDeltaTime += (time - m_LastTime) / m_LimitFPS;
		m_LastTime = time;
	}

	void Application::EndFrame()
	{
		m_Frames++;

		// Reset after one second.
		if (m_Window->GetTime() - m_Timer > 1.0)
		{
			// Display frame rate at window bar.
			std::stringstream ss;
			ss << m_Specification.Name << " [" << m_Frames << " FPS]";
			m_Window->SetTitle(ss.str());

			// Log framerate and delta time to console.
			printf("FPS: %d, DeltaTime: %f\n", m_Frames, m_DeltaTime);

			m_Timer++;
			m_Frames = 0;
			m_FixedUpdates = 0;
		}
	}

	void Application::Start()
	{
		m_Window->Create();

		// Initialize the renderer.
		Renderer2D::Get().Initialize(m_Specification.GraphicsAPI);

		glm::vec3 position = { 10.f, 10.f, 0.f };
		glm::vec3 scale = { 10.f, 10.f, 0.f };
		glm::vec4 color = { 1.f, 0.2f, 0.1f, 1.f };

		m_Window->DispatchMainLoop([&]()
		{
			BeginFrame();

			position.x = 5.f;
			position.y = 5.f;
			scale = { 10.f, 10.f, 0.f };
			for (int i = 0; i < 99; i++)
			{
				for (int j = 0; j < 99; j++)
				{
					Renderer2D::Get().DrawQuad(0, position, scale, 0.f);
					position.x += 15.f;
				}
				position.x = 5.f;
				position.y += 15.f;
			}

			position.x = 12.5f;
			position.y = 12.5f;
			color = { 1.f, 0.2f, 0.1f, 1.f };

			for (int i = 0; i < 99; i++)
			{
				for (int j = 0; j < 99; j++)
				{
					Renderer2D::Get().DrawQuad(1, position, scale, 0.f, color);
					position.x += 15.f;
				}
				position.x = 12.5f;
				position.y += 15.f;
			}

			position.x = 5.f;
			position.y = 5.f;
			scale = { 5.f, 5.f, 0.f };
			color = { 0.f, 1.f, 0.f, 1.f };
			for (int i = 0; i < 99; i++)
			{
				for (int j = 0; j < 299; j++)
				{
					Renderer2D::Get().DrawQuad(2, position, scale, 0.f, color);
					position.x += 10.f;
				}
				position.x = 5.f;
				position.y += 10.f;
			}

			position.x = 5.f;
			position.y = 5.f;
			scale = { 5.f, 5.f, 0.f };
			color = { 0.f, 1.f, 1.f, 1.f };
			for (int i = 0; i < 99; i++)
			{
				for (int j = 0; j < 299; j++)
				{
					Renderer2D::Get().DrawQuad(3, position, scale, 0.f, color);
					position.x += 10.f;
				}
				position.x = 5.f;
				position.y += 10.f;
			}

			position.x = 5.f;
			position.y = 5.f;
			scale = { 5.f, 5.f, 0.f };
			color = { 1.f, 0.f, 1.f, 1.f };
			for (int i = 0; i < 100; i++)
			{
				for (int j = 0; j < 300; j++)
				{
					Renderer2D::Get().DrawQuad(3, position, scale, 0.f, color);
					position.x += 10.f;
				}
				position.x = 5.f;
				position.y += 10.f;
			}

			Renderer2D::Get().BeginFrame();
			Renderer2D::Get().Submit();
			Renderer2D::Get().EndFrame();

			EndFrame();
		});

		Shutdown();
	}

	void Application::Shutdown()
	{
		m_Window->Terminate();
	}
}