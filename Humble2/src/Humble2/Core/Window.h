#pragma once

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

namespace HBL
{
	class Window
	{
	public:
		Window() = default;
		Window(const std::string& title, float width, float height, bool fullScreen, bool vSync);

		GLFWwindow* GetHandle();

		void Create();
		void DispatchMainLoop(std::function<void()> mainLoop);
		double GetTime();
		void Close();
		void Terminate();

	private:
		GLFWwindow* m_Window;
		std::string m_Title;
		float m_RefreshRate;
		float m_Width;
		float m_Height;
		bool m_FullScreen;
		bool m_VSync;

		void DispatchMainEm(void* fp);
	};
}