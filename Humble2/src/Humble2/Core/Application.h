#pragma once

#include "Window.h"
#include "Platform\OpenGL\Rewrite\OpenGLWindow.h"

#include "Base.h"
#include "Input.h"
#include "Context.h"

#include "ImGui\ImGuiRenderer.h"
#include "Platform\OpenGL\Rewrite\OpenGLImGuiRenderer.h"

#include "Systems\CameraSystem.h"
#include "Systems\StaticMeshRenderingSystem.h"
#include "Systems\TransformSystem.h"
#include "Systems\LinkSystem.h"

#include "Utilities\AssetManager.h"
#include "Resources\ResourceManager.h"
#include "Platform\OpenGL\Rewrite\OpenGLResourceManager.h"
#include "Renderer\Rewrite\Renderer.h"
#include "Platform\OpenGL\Rewrite\OpenGLRenderer.h"
#include "Renderer\Rewrite\Device.h"
#include "Platform\OpenGL\Rewrite\OpenGLDevice.h"

#include <string>
#include <sstream>
#include <iostream>

namespace HBL2
{
	enum class Platform
	{
		Windows,
		Web,
		None
	};

	struct ApplicationSpec
	{
		std::string Name = "Humble2 Application";
		std::string CommandLineArgs = "";
		GraphicsAPI GraphicsAPI = GraphicsAPI::OPENGL;
		float Width = 1920.f;
		float Height = 1080.f;
		bool VerticalSync = true;
		bool Fullscreen = false;

		Context* Context = nullptr;
	};

	class Application
	{
	public:
		Application(ApplicationSpec& specification);
		~Application() = default;

		void Start();

		static Application& Get()
		{
			return *s_Instance;
		}

		const ApplicationSpec& GetSpec() const { return m_Specification; }

	private:
		static Application* s_Instance;

		ApplicationSpec m_Specification;

		float m_LastTime = 0.0f;
		float m_Timer = m_LastTime;
		float m_DeltaTime = 0.0f;
		int m_Frames = 0, m_Updates = 0;

		float m_LimitFPS = 1.0f / 60.0f;
		float m_FixedDeltaTime = 0.0f;
		int m_FixedUpdates = 0;

		void BeginFrame();
		void EndFrame();
		void Shutdown();
	};
}