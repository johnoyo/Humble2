#include "Window.h"

namespace HBL
{
	void WindowResizeCallback(GLFWwindow* window, int width, int height)
	{
		Renderer2D::Get().SetViewport(0, 0, width, height);
	}

	Window::Window(const std::string& title, float width, float height, bool fullScreen, bool vSync) :
		m_Title(title), m_Width(width), m_Height(height), m_FullScreen(fullScreen), m_VSync(vSync)
	{
		m_Window = nullptr;
		m_RefreshRate = 0.f;
	}

	GLFWwindow* Window::GetHandle()
	{
		return m_Window;
	}

	void Window::Create()
	{
		if (!glfwInit()) 
		{
			std::cout << "Error initializing window!\n";
			exit(-1);
		}

		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

		m_RefreshRate = (float)mode->refreshRate;

		if (m_FullScreen)
		{
			m_Window = glfwCreateWindow(mode->width, mode->height, m_Title.c_str(), glfwGetPrimaryMonitor(), NULL);
		}
		else
		{
			m_Window = glfwCreateWindow((int)m_Width, (int)m_Height, m_Title.c_str(), NULL, NULL);
		}

		if (!m_Window)
		{
			std::cout << "Error creating window!\n";
			glfwTerminate();
			exit(-1);
		}

		if (m_VSync)
		{
			glfwMakeContextCurrent(m_Window);
			glfwSwapInterval(1);
		}
		else
		{
			glfwMakeContextCurrent(m_Window);
			glfwSwapInterval(0);
		}

#ifndef EMSCRIPTEN
		if (glewInit() != GLEW_OK)
		{
			std::cout << "Error initializing GLEW!\n";
			exit(-1);
		}
#endif
		glfwSetWindowSizeCallback(m_Window, WindowResizeCallback);
	}

	void Window::DispatchMainEm(void* fp)
	{
		std::function<void()>* func = (std::function<void()>*)fp;
		(*func)();
	}

	void Window::DispatchMainLoop(std::function<void()> mainLoop)
	{
#ifdef EMSCRIPTEN
		std::function<void()> mainLoopEm = [&]()
		{
			mainLoop();

			glfwSwapBuffers(m_Window);
			glfwPollEvents();
		};
		emscripten_set_main_loop_arg(DispatchMainEm, &mainLoopEm, 0, 1);
#else
		while (!glfwWindowShouldClose(m_Window))
		{
			mainLoop();

			glfwSwapBuffers(m_Window);
			glfwPollEvents();
		}
#endif
	}

	double Window::GetTime()
	{
		return glfwGetTime();
	}

	void Window::Close()
	{
		glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
	}

	void Window::Terminate()
	{
		glfwTerminate();
	}
}