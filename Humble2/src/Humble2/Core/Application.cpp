#include "Application.h"

namespace HBL2
{
	Application* Application::s_Instance = nullptr;

	Application::Application(ApplicationSpec& specification) : m_Specification(specification)
	{
		HBL2_CORE_ASSERT(!s_Instance, "Application already exists!");

		s_Instance = this;

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

		// Initialize empty scene.
		m_Specification.Context->EmptyScene = new Scene("EmptyScene");

		m_Specification.Context->Core = new Scene("Core");
		//m_Specification.Context->Core->RegisterSystem(new SpriteRendererSystem);
		//m_Specification.Context->Core->RegisterSystem(new MeshRendererSystem);
		m_Specification.Context->Core->RegisterSystem(new TransformSystem);
		m_Specification.Context->Core->RegisterSystem(new LinkSystem);
		m_Specification.Context->Core->RegisterSystem(new TestRendererSystem);
		m_Specification.Context->Core->RegisterSystem(new CameraSystem);
	}

	Application::~Application()
	{
		// ImGuiRenderer::Instance->Clean();
		// delete ImGuiRenderer::Instance;

		Renderer::Instance->Clean();
		delete Renderer::Instance;

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

		if (m_Specification.GraphicsAPI == GraphicsAPI::OpenGL)
		{
			glfwSwapBuffers(m_Window->GetHandle());
		}
	}

	void Application::Start()
	{
		// Create window.
		m_Window->Create();

		HBL::ResourceManager::Instance = new HBL::OpenGLResourceManager();
		Renderer::Instance = new OpenGLRenderer();
		ImGuiRenderer::Instance = new OpenGLImGuiRenderer();

		Input::SetWindow(m_Window->GetHandle());

		m_Specification.Context->OnAttach();

		// Initialize the renderers.
		Renderer::Instance->Initialize();
		//ImGuiRenderer::Instance->Initialize(m_Window);

		m_Specification.Context->OnCreate();

		m_Window->DispatchMainLoop([&]()
		{
			BeginFrame();

			Renderer::Instance->BeginFrame();
			m_Specification.Context->OnUpdate(m_DeltaTime);
			Renderer::Instance->EndFrame();

			//ImGuiRenderer::Instance->BeginFrame();
			//m_Specification.Context->OnGuiRender(m_DeltaTime);
			//ImGuiRenderer::Instance->EndFrame();

			EndFrame();
		});

		Shutdown();
	}

	void Application::Shutdown()
	{
		m_Window->Terminate();
	}
}