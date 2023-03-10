#include "Application.h"

namespace HBL
{
	Application::Application(ApplicationSpec& spec) : m_Spec(spec)
	{
		switch (m_Spec.Platform)
		{
		case Platform::Windows:
			break;
		case Platform::Web:
			break;
		case Platform::None:
			std::cout << "No target platform specified. Please choose between Windows or Web.\n";
			exit(-1);
			break;
		default:
			break;
		}

		switch (m_Spec.Core)
		{
		case Core::Humble2D:
			break;
		case Core::Humble3D:
			break;
		case Core::None:
			std::cout << "No Core template specified. Please choose between 2D or 3D core.\n";
			exit(-1);
			break;
		default:
			break;
		}

		switch (m_Spec.RendererAPI)
		{
		case RendererAPI::OpenGLES:
			break;
		case RendererAPI::OpenGL:
			break;
		case RendererAPI::Vulkan:
			break;
		case RendererAPI::None:
			std::cout << "No RendererAPI specified. Please choose between OpenGL, OpenGLES or Vulkan depending on your target platform.\n";
			exit(-1);
			break;
		default:
			break;
		}

		m_Window = new Window(m_Spec.Name, m_Spec.Width, m_Spec.Height, m_Spec.Fullscreen, m_Spec.Vsync);
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