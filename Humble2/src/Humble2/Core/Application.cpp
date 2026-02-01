#include "Application.h"

#include "Asset\EditorAssetManager.h"
#include "Script\BuildEngine.h"
#include "Platform\Windows\WindowsBuildEngine.h"

#ifdef DIST
	#define BEGIN_APP_PROFILE(tag)
	#define END_APP_PROFILE(tag, time)
	#define SWAP_AND_RESET_PROFILED_TIMERS()
#else
	#define BEGIN_APP_PROFILE(tag) Timer tag
	#define END_APP_PROFILE(tag, time) time = tag.ElapsedMillis()
	#define SWAP_AND_RESET_PROFILED_TIMERS() (m_PreviousStats = m_CurrentStats, m_CurrentStats.Reset())
#endif

#define SKIP_MT_FRAME() if (skipMTFrame.load() <= 0) { if (skipMTFrame.load() == 0) { skipMTFrame = 1; } EndFrame(); return; }
#define SKIP_RT_FRAME() if (skipRTFrame.load() <= 0) { if (skipRTFrame.load() == 0) { skipRTFrame = 1; } continue; }

#define MULTITHREADING 1

namespace HBL2
{
	Application* Application::s_Instance = nullptr;

	static const char* g_GfxAPI;
	static std::atomic_int32_t skipMTFrame = { 0 };
	static std::atomic_int32_t skipRTFrame = { 0 };

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

		Allocator::Arena.Initialize(500_MB, 1_MB);
		Allocator::FrameArena.Initialize(&Allocator::Arena, 50_MB);

		Allocator::Frame.Initialize(4_MB);
		Allocator::Persistent.Initialize(16_MB);

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

		BuildEngine::Instance = new WindowsBuildEngine;

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

		AssetManager::Instance->Initialize();
		ResourceManager::Instance->Initialize();

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

		EventDispatcher::Get().Register<WindowIconifyEvent>([](const WindowIconifyEvent& e)
		{
			if (e.Iconified)
			{
				skipMTFrame = -1;
				skipRTFrame = -1;
			}
			else
			{
				skipMTFrame = 1;
				skipRTFrame = 1;
			}
		});
	}

	Application::~Application()
	{
		Allocator::Frame.Free();
		Allocator::Persistent.Free();
	}

	void Application::DispatchRenderLoop(const std::function<void()>& renderLoop)
	{
		m_RenderThread = std::thread(renderLoop);

		auto handle = m_RenderThread.native_handle();

#ifdef _WIN32
		// Put render thread on to dedicated core.
		DWORD_PTR affinityMask = 1ull << 1;
		DWORD_PTR affinity_result = SetThreadAffinityMask(handle, affinityMask);
		assert(affinity_result > 0);

		// Set priority to normal.
		BOOL priority_result = SetThreadPriority(handle, THREAD_PRIORITY_NORMAL);
		assert(priority_result != 0);

		// Give a name to render thread for easier debugging.
		std::wstring wthreadname = L"HBL2::RenderThread";
		HRESULT hr = SetThreadDescription(handle, wthreadname.c_str());
		assert(SUCCEEDED(hr));
#endif
	}

	void Application::WaitForRenderThreadInitialization()
	{
		while (!m_RenderThreadInitializationFinished.load(std::memory_order_acquire))
		{
			// no-op
		}

		// Setup a render context for main thread.
		Device::Instance->SetContext(ContextType::FETCH);
	}

	void Application::WaitForRenderThreadShutdown()
	{
		Renderer::Instance->ShutdownRenderThread();

		if (m_RenderThread.joinable())
		{
			m_RenderThread.join();
		}
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
			// Ensure current frame is done rendering before resetting.
			Renderer::Instance->WaitForRenderThreadIdle();

			// Reset renderer for scene change.
			Renderer::Instance->ResetForSceneChange();

			// Begin the scene load.
			SceneManager::Get().LoadSceneDeffered();
		}

		// Reset frame allocator.
		Allocator::Frame.Invalidate();
		Allocator::FrameArena.Reset();

		SWAP_AND_RESET_PROFILED_TIMERS();
	}

	void Application::Start()
	{
		const auto& projectSettings = Project::GetActive()->GetSpecification().Settings;

		BuildEngine::Instance->Initialize();

		Window::Instance->Create();
		ImGuiRenderer::Instance->Create({ .EnableMultiViewports = projectSettings.EditorMultipleViewports });
		
		Input::Initialize();

#if !MULTITHREADING
		Device::Instance->Initialize();
		Renderer::Instance->Initialize();
		ImGuiRenderer::Instance->Initialize();
		DebugRenderer::Instance->Initialize();

		TextureUtilities::Get().LoadWhiteTexture();
		ShaderUtilities::Get().LoadBuiltInShaders();
		ShaderUtilities::Get().LoadBuiltInMaterials();
		MeshUtilities::Get().LoadBuiltInMeshes();

		m_Specification.Context->OnCreate();

		int frameIndex = 0;

		Window::Instance->DispatchMainLoop([this, &frameIndex]()
		{
			BEGIN_APP_PROFILE(gameThread);

			BeginFrame();

			if (frameIndex == 0)
			{
				frameIndex++;
				EndFrame();
				return;
			}

			BEGIN_APP_PROFILE(debugDraw);
			DebugRenderer::Instance->BeginFrame();
			m_Specification.Context->OnGizmoRender(Time::DeltaTime);
			DebugRenderer::Instance->EndFrame();
			END_APP_PROFILE(debugDraw, m_CurrentStats.DebugDrawTime);

			BEGIN_APP_PROFILE(appUpdate);
			m_Specification.Context->OnUpdate(Time::DeltaTime);
			m_Specification.Context->OnFixedUpdate();
			END_APP_PROFILE(appUpdate, m_CurrentStats.AppUpdateTime);

			BEGIN_APP_PROFILE(appGUIDraw);
			ImGuiRenderer::Instance->BeginFrame();
			m_Specification.Context->OnGuiRender(Time::DeltaTime);
			ImGuiRenderer::Instance->EndFrame();
			END_APP_PROFILE(appGUIDraw, m_CurrentStats.AppGuiDrawTime);

			Renderer::Instance->WaitAndSubmit();

			EndFrame();

			END_APP_PROFILE(gameThread, m_CurrentStats.GameThreadTime);

			// -------------------------------------------------------------------

			BEGIN_APP_PROFILE(renderThread);

			const FrameData2* frameData = Renderer::Instance->WaitAndRender();

			if (frameData == nullptr)
			{
				return;
			}

			BEGIN_APP_PROFILE(render);
			Renderer::Instance->BeginFrame();
			Renderer::Instance->Render(*frameData);
			ImGuiRenderer::Instance->Render(*frameData);
			Renderer::Instance->EndFrame();
			END_APP_PROFILE(render, m_CurrentStats.RenderTime);

			BEGIN_APP_PROFILE(present);
			Renderer::Instance->Present();
			END_APP_PROFILE(present, m_CurrentStats.PresentTime);

			END_APP_PROFILE(renderThread, m_CurrentStats.RenderThreadTime);
		});
#else
		DispatchRenderLoop([this]()
		{
			Device::Instance->Initialize();
			Renderer::Instance->Initialize();
			ImGuiRenderer::Instance->Initialize();
			DebugRenderer::Instance->Initialize();

			TextureUtilities::Get().LoadWhiteTexture();
			ShaderUtilities::Get().LoadBuiltInShaders();
			ShaderUtilities::Get().LoadBuiltInMaterials();
			MeshUtilities::Get().LoadBuiltInMeshes();

			m_RenderThreadInitializationFinished.store(true, std::memory_order_release);

			while (true)
			{
				SKIP_RT_FRAME();

				BEGIN_APP_PROFILE(renderThread);

				BEGIN_APP_PROFILE(renderThreadWait);
				const FrameData* frameData = Renderer::Instance->WaitAndRender();
				END_APP_PROFILE(renderThreadWait, m_CurrentStats.RenderThreadWaitTime);

				if (frameData == nullptr) { break; }

				BEGIN_APP_PROFILE(render);
				Renderer::Instance->BeginFrame();
				Renderer::Instance->Render(*frameData);
				ImGuiRenderer::Instance->Render(*frameData);
				Renderer::Instance->EndFrame();
				END_APP_PROFILE(render, m_CurrentStats.RenderTime);

				BEGIN_APP_PROFILE(present);
				Renderer::Instance->Present();
				Renderer::Instance->ReleaseFrameSlot(frameData->AcquiredIndex);
				END_APP_PROFILE(present, m_CurrentStats.PresentTime);

				END_APP_PROFILE(renderThread, m_CurrentStats.RenderThreadTime);
			}

			TextureUtilities::Get().DeleteWhiteTexture();
			ShaderUtilities::Get().DeleteBuiltInShaders();
			ShaderUtilities::Get().DeleteBuiltInMaterials();
			MeshUtilities::Get().DeleteBuiltInMeshes();
		});

		WaitForRenderThreadInitialization();

		m_Specification.Context->OnCreate();

		Window::Instance->DispatchMainLoop([this]()
		{
			BEGIN_APP_PROFILE(gameThread);

			BeginFrame();

			SKIP_MT_FRAME();

			BEGIN_APP_PROFILE(gameThreadWait);
			Renderer::Instance->WaitAndBegin();
			END_APP_PROFILE(gameThreadWait, m_CurrentStats.GameThreadWaitTime);

			BEGIN_APP_PROFILE(debugDraw);
			DebugRenderer::Instance->BeginFrame();
			m_Specification.Context->OnGizmoRender(Time::DeltaTime);
			DebugRenderer::Instance->EndFrame();
			END_APP_PROFILE(debugDraw, m_CurrentStats.DebugDrawTime);

			BEGIN_APP_PROFILE(appUpdate);
			m_Specification.Context->OnUpdate(Time::DeltaTime);
			m_Specification.Context->OnFixedUpdate();
			END_APP_PROFILE(appUpdate, m_CurrentStats.AppUpdateTime);

			BEGIN_APP_PROFILE(appGUIDraw);
			ImGuiRenderer::Instance->BeginFrame();
			m_Specification.Context->OnGuiRender(Time::DeltaTime);
			ImGuiRenderer::Instance->EndFrame();
			END_APP_PROFILE(appGUIDraw, m_CurrentStats.AppGuiDrawTime);

			Renderer::Instance->MarkAndSubmit();

			EndFrame();

			END_APP_PROFILE(gameThread, m_CurrentStats.GameThreadTime);
		});
#endif
		Renderer::Instance->WaitForRenderThreadIdle();

		m_Specification.Context->OnDestroy();

		m_Specification.Context->OnDetach();

		WaitForRenderThreadShutdown();

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
		
		BuildEngine::Instance->ShutDown();
		delete BuildEngine::Instance;
		BuildEngine::Instance = nullptr;

		Input::ShutDown();
		ShaderUtilities::Shutdown();
		MeshUtilities::Shutdown();
		EventDispatcher::Shutdown();
		JobSystem::Shutdown();
	}
}