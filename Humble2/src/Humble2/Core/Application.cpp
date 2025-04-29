#include "Application.h"

namespace HBL2
{
	Application* Application::s_Instance = nullptr;

	static const char* g_GfxAPI;

	Application::Application(ApplicationSpec& specification) : m_Specification(specification)
	{
		HBL2_CORE_ASSERT(!s_Instance, "Application already exists!");

		s_Instance = this;

		Allocator::Frame.Initialize(32_MB);
		Allocator::Scene.Initialize(256_MB);
		Allocator::App.Initialize(256_MB);

		Log::Initialize();
		Random::Initialize();
		EventDispatcher::Initialize();
		JobSystem::Initialize();

		MeshUtilities::Initialize();
		NativeScriptUtilities::Initialize();
		UnityBuild::Initialize();

		switch (m_Specification.GraphicsAPI)
		{
		case GraphicsAPI::OPENGL:
			HBL2_CORE_INFO("OpenGL is selected as the renderer API.");
			g_GfxAPI = "OpenGL";
			Device::Instance = new OpenGLDevice;
			Window::Instance = new OpenGLWindow;
			ResourceManager::Instance = new OpenGLResourceManager;
			Renderer::Instance = new OpenGLRenderer;
			ImGuiRenderer::Instance = new OpenGLImGuiRenderer;
			break;
		case GraphicsAPI::VULKAN:
			g_GfxAPI = "Vulkan";
			HBL2_CORE_INFO("Vulkan is selected as the renderer API.");
			Device::Instance = new VulkanDevice;
			Window::Instance = new VulkanWindow;
			ResourceManager::Instance = new VulkanResourceManager;
			Renderer::Instance = new VulkanRenderer;
			ImGuiRenderer::Instance = new VulkanImGuiRenderer;
			break;
		case GraphicsAPI::NONE:
			HBL2_CORE_ERROR("No RendererAPI specified. Please choose between OpenGL, or Vulkan depending on your target platform.");
			exit(-1);
			break;
		default:
			break;
		}

		Window::Instance->Initialize({
			.Title = m_Specification.Name,
			.Width = m_Specification.Width,
			.Height = m_Specification.Height,
			.FullScreen = m_Specification.Fullscreen,
			.VerticalSync = m_Specification.VerticalSync
		});

		m_Specification.Context->EmptyScene = ResourceManager::Instance->CreateScene({ .name = "Empty Scene" });
		m_Specification.Context->EditorScene = ResourceManager::Instance->CreateScene({ .name = "Editor Scene" });
		
		ShaderUtilities::Initialize();
	}

	Application::~Application()
	{
		Allocator::Frame.Free();
		Allocator::Scene.Free();
		Allocator::App.Free();
	}

	void Application::BeginFrame()
	{
		float time = (float)Window::Instance->GetTime();
		Time::DeltaTime = time - m_LastTime;
		Time::FixedDeltaTime += (time - m_LastTime) / Time::FixedTimeStep;
		m_LastTime = time;
	}

	void Application::EndFrame()
	{
		m_Frames++;

		if (Window::Instance->GetTime() - m_Timer > 1.0)
		{
			Window::Instance->SetTitle(std::format("{} [{}] FPS ({})", m_Specification.Name, m_Frames, g_GfxAPI));

			HBL2_CORE_TRACE("FPS: {0}, DeltaTime: {1} ({2})", m_Frames, Time::DeltaTime, g_GfxAPI);

			m_Timer++;
			m_Frames = 0;
			m_FixedUpdates = 0;
		}

		if (SceneManager::Get().SceneChangeRequested)
		{
			// Reset scene allocator.
			Allocator::Scene.Invalidate();

			SceneManager::Get().LoadSceneDeffered();
		}

		// Reset frame allocator.
		Allocator::Frame.Invalidate();
	}

	void Application::Start()
	{
		Window::Instance->Create();
		
		Input::Initialize();

		Device::Instance->Initialize();
		Renderer::Instance->Initialize();
		ImGuiRenderer::Instance->Initialize();

		m_Specification.Context->OnCreate();

		Window::Instance->DispatchMainLoop([&]()
		{
			BeginFrame();

			Renderer::Instance->BeginFrame();
			m_Specification.Context->OnUpdate(Time::DeltaTime);
			m_Specification.Context->OnFixedUpdate();
			Renderer::Instance->EndFrame();

			/*DebugRenderer::Instance->BeginFrame();
			m_Specification.Context->OnGizmoRender(Time::DeltaTime);
			DebugRenderer::Instance->EndFrame();*/

			ImGuiRenderer::Instance->BeginFrame();
			m_Specification.Context->OnGuiRender(Time::DeltaTime);
			ImGuiRenderer::Instance->EndFrame();

			Renderer::Instance->Present();

			EndFrame();
		});

		m_Specification.Context->OnDestroy();

		Shutdown();
	}

	void Application::Shutdown()
	{

		ImGuiRenderer::Instance->Clean();
		delete ImGuiRenderer::Instance;
		ImGuiRenderer::Instance = nullptr;

		AssetManager::Instance->DeregisterAssets();
		delete AssetManager::Instance;
		AssetManager::Instance = nullptr;

		Renderer::Instance->Clean();
		delete Renderer::Instance;
		Renderer::Instance = nullptr;

		ResourceManager::Instance->Clean();
		delete ResourceManager::Instance;
		ResourceManager::Instance = nullptr;

		Device::Instance->Destroy();
		delete Device::Instance;
		Device::Instance = nullptr;

		Window::Instance->Terminate();
		delete Window::Instance;
		Window::Instance = nullptr;

		Input::ShutDown();
		UnityBuild::Shutdown();
		NativeScriptUtilities::Shutdown();
		ShaderUtilities::Shutdown();
		MeshUtilities::Shutdown();
		EventDispatcher::Shutdown();
		JobSystem::Shutdown();
	}
}