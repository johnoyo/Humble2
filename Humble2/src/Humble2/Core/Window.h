#pragma once

#include "Events.h"
#include "EventDispatcher.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <emscripten\emscripten.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>

#include <string>
#include <iostream>
#include <functional>

namespace HBL2
{
	struct WindowSpecification
	{
		std::string Title = "Humble App";
		float RefreshRate = 0.0f;
		float Width = 1920.0f;
		float Height = 1080.0f;
		bool FullScreen = false;
		bool VerticalSync = false;
	};

	class Window
	{
	public:
		~Window() = default;
		static inline Window* Instance;

		void Initialize(const WindowSpecification&& spec);

		virtual void Create() = 0;
		virtual void Present() = 0;

		void DispatchMainLoop(const std::function<void()>& mainLoop);
		void SetTitle(const std::string& title);
		double GetTime();
		void Close();
		GLFWwindow* GetHandle();
		void Terminate();

	protected:
		void AttachEventCallbacks();

	protected:
		GLFWwindow* m_Window;
		WindowSpecification m_Spec;

		static void DispatchMainEm(void* fp);
	};
}