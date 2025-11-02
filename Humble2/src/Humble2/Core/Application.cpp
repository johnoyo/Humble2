#include "Application.h"

#include "Asset\EditorAssetManager.h"

#ifdef DIST
	#define BEGIN_APP_PROFILE(tag)
	#define END_APP_PROFILE(tag, time)
	#define SWAP_AND_RESET_PROFILED_TIMERS()
#else
	#define BEGIN_APP_PROFILE(tag) Timer tag
	#define END_APP_PROFILE(tag, time) time = tag.ElapsedMillis()
	#define SWAP_AND_RESET_PROFILED_TIMERS() (m_PreviousStats = m_CurrentStats, m_CurrentStats.Reset())
#endif

namespace HBL2
{
	Application* Application::s_Instance = nullptr;

	static const char* g_GfxAPI;

	Application::Application(ApplicationSpec& specification) : m_Specification(specification)
	{
		HBL2_CORE_ASSERT(!s_Instance, "Application already exists!");

		s_Instance = this;

		Log::Initialize();
		Random::Initialize();

		m_Specification.Context->OnAttach();

		if (Project::GetActive() == nullptr)
		{
			exit(-1);
		}

		const auto& projectSettings = Project::GetActive()->GetSpecification().Settings;

		Allocator::Frame.Initialize(32_MB);
		Allocator::Persistent.Initialize(256_MB);

		GraphicsAPI gfxAPI = GraphicsAPI::NONE;

		switch (Context::Mode)
		{
		case Mode::Editor:
			AssetManager::Instance = new EditorAssetManager;
			gfxAPI = projectSettings.EditorGraphicsAPI;
			break;
		case Mode::Runtime:
			AssetManager::Instance = new EditorAssetManager; // TODO: Change to RuntimeAssetManager when implemented.
			gfxAPI = projectSettings.RuntimeGraphicsAPI;
			break;
		}

		EventDispatcher::Initialize();
		JobSystem::Initialize();
		MeshUtilities::Initialize();
		NativeScriptUtilities::Initialize();
		UnityBuild::Initialize();

		switch (gfxAPI)
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
			HBL2_CORE_INFO("Vulkan is selected as the renderer API.");
			g_GfxAPI = "Vulkan";
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

		DebugRenderer::Instance = new DebugRenderer;

		switch (projectSettings.Physics2DImpl)
		{
		case Physics2DEngineImpl::BOX2D:
		case Physics2DEngineImpl::CUSTOM:
			PhysicsEngine2D::Instance = new Box2DPhysicsEngine;
			break;
		}

		switch (projectSettings.Physics3DImpl)
		{
		case Physics3DEngineImpl::JOLT:
		case Physics3DEngineImpl::CUSTOM:
			PhysicsEngine3D::Instance = new JoltPhysicsEngine;
			break;
		}
	}

	Application::~Application()
	{
		Allocator::Frame.Free();
		Allocator::Persistent.Free();
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

			HBL2_CORE_TRACE("FPS: {0}, DeltaTime: {1} ({2})", m_Frames, Time::DeltaTime * 1000.0f, g_GfxAPI);

			m_Timer++;
			m_Frames = 0;
			m_FixedUpdates = 0;
		}

		if (SceneManager::Get().SceneChangeRequested)
		{
			SceneManager::Get().LoadSceneDeffered();
		}

		// Reset frame allocator.
		Allocator::Frame.Invalidate();

		SWAP_AND_RESET_PROFILED_TIMERS();
	}

	void Application::Start()
	{
		Window::Instance->Create();
		ImGuiRenderer::Instance->Create();
		
		Input::Initialize();

		Device::Instance->Initialize();
		Renderer::Instance->Initialize();
		ImGuiRenderer::Instance->Initialize();
		DebugRenderer::Instance->Initialize();

		m_Specification.Context->OnCreate();

		Window::Instance->DispatchMainLoop([&]()
		{
			BeginFrame();

			BEGIN_APP_PROFILE(debugDraw);
			DebugRenderer::Instance->BeginFrame();
			m_Specification.Context->OnGizmoRender(Time::DeltaTime);
			DebugRenderer::Instance->EndFrame();
			END_APP_PROFILE(debugDraw, m_CurrentStats.DebugDrawTime);

			BEGIN_APP_PROFILE(appUpdate);
			Renderer::Instance->BeginFrame();
			m_Specification.Context->OnUpdate(Time::DeltaTime);
			m_Specification.Context->OnFixedUpdate();
			Renderer::Instance->EndFrame();
			END_APP_PROFILE(appUpdate, m_CurrentStats.AppUpdateTime);

			BEGIN_APP_PROFILE(appGUIDraw);
			ImGuiRenderer::Instance->BeginFrame();
			m_Specification.Context->OnGuiRender(Time::DeltaTime);
			ImGuiRenderer::Instance->EndFrame();
			END_APP_PROFILE(appGUIDraw, m_CurrentStats.AppGuiDrawTime);

			BEGIN_APP_PROFILE(present);
			Renderer::Instance->Present();
			END_APP_PROFILE(present, m_CurrentStats.PresentTime);

			EndFrame();
		});

		m_Specification.Context->OnDestroy();

		m_Specification.Context->OnDetach();

		Shutdown();
	}

	void Application::Shutdown()
	{
		ImGuiRenderer::Instance->Clean();
		delete ImGuiRenderer::Instance;
		ImGuiRenderer::Instance = nullptr;

		DebugRenderer::Instance->Clean();
		delete DebugRenderer::Instance;
		DebugRenderer::Instance = nullptr;

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