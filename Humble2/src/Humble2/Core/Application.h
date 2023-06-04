#pragma once

#include "Window.h"
#include "Base.h"
#include "Renderer/Renderer2D.h"

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
		std::string Name = "Humble Application";
		Platform Platform = Platform::Windows;
		GraphicsAPI GraphicsAPI = GraphicsAPI::OpenGL;
		float Width = 960.f;
		float Height = 540.f;
		bool Vsync = true;
		bool Fullscreen = false;
	};

	class Application
	{
	public:
		Application(ApplicationSpec& specification);
		~Application();

		void Start();

	private:
		ApplicationSpec m_Specification;
		Window* m_Window;

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