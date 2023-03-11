#include "Application.h"

namespace HBL
{
	Application::Application(ApplicationSpec& spec) : m_Spec(spec)
	{
		Log::Initialize();

		switch (m_Spec.Platform)
		{
		case Platform::Windows:
			HBL_CORE_INFO("Windows platform selected.");
			break;
		case Platform::Web:
			HBL_CORE_INFO("Web platform selected.");
			break;
		case Platform::None:
			HBL_CORE_ERROR("No target platform specified. Please choose between Windows or Web.");
			exit(-1);
			break;
		default:
			break;
		}

		switch (m_Spec.Core)
		{
		case Core::Humble2D:
			HBL_CORE_INFO("Humble 2D is selected as core.");
			break;
		case Core::Humble3D:
			HBL_CORE_INFO("Humble 3D is selected as core.");
			break;
		case Core::None:
			HBL_CORE_ERROR("No Core template specified. Please choose between 2D or 3D core.");
			exit(-1);
			break;
		default:
			break;
		}

		switch (m_Spec.RendererAPI)
		{
		case GraphicsAPI::OpenGL:
			HBL_CORE_INFO("OpenGL is selected as the renderer API.");
			break;
		case GraphicsAPI::Vulkan:
			if (m_Spec.Platform == Platform::Web)
			{
				HBL_CORE_WARN("Web is the selected platform, defaulting to OpenGLES as the renderer API.");
				m_Spec.RendererAPI = GraphicsAPI::OpenGL;
				break;
			}
			HBL_CORE_INFO("Vulkan is selected as the renderer API.");
			break;
		case GraphicsAPI::None:
			HBL_CORE_ERROR("No RendererAPI specified. Please choose between OpenGL, or Vulkan depending on your target platform.");
			exit(-1);
			break;
		default:
			break;
		}

		// Intialize the window.
		m_Window = new Window(m_Spec.Name, m_Spec.Width, m_Spec.Height, m_Spec.Fullscreen, m_Spec.Vsync);

		// Initialize the renderer.
		Renderer2D::Get().Initialize(m_Spec.RendererAPI);
	}

	Application::~Application()
	{
		delete m_Window;
	}

	void Application::Start()
	{
		m_Window->Create();

		m_Window->DispatchMainLoop([&]()
		{
			// Measure time and delta time
			float time = (float)glfwGetTime();
			m_DeltaTime = time - m_LastTime;
			m_FixedDeltaTime += (time - m_LastTime) / m_LimitFPS;
			m_LastTime = time;

			glClear(GL_COLOR_BUFFER_BIT);
			glClearColor(1.f, 0.7f, 0.3f, 1.0f);

			m_Frames++;

			// Reset after one second
			if (glfwGetTime() - m_Timer > 1.0)
			{
				// Display frame rate at window bar
				std::stringstream ss;
				ss << m_Spec.Name << " [" << m_Frames << " FPS]";
				glfwSetWindowTitle(m_Window->GetHandle(), ss.str().c_str());

				// Log framerate and delta time to console
				std::cout << "FPS: " << m_Frames << ", DeltaTime: " << m_DeltaTime << "\n";

				m_Timer++;
				m_Frames = 0;
				m_FixedUpdates = 0;
			}
		});

		Shutdown();
	}

	void Application::Shutdown()
	{
		m_Window->Terminate();
	}
}