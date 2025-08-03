#include "Window.h"

namespace HBL2
{
	Window* Window::Instance = nullptr;

	static void WindowCloseCallback(GLFWwindow* window)
	{
		EventDispatcher::Get().Post(WindowCloseEvent());
	}

	static void WindowSizeCallback(GLFWwindow* window, int width, int height)
	{
		Window::Instance->SetExtents(width, height);
		EventDispatcher::Get().Post(WindowSizeEvent(width, height));
	}

	static void WindowPositionCallback(GLFWwindow* window, int xpos, int ypos)
	{
		Window::Instance->SetPosition(xpos, ypos);
		EventDispatcher::Get().Post(WindowPositionEvent(xpos, ypos));
	}

	static void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
	{
		Window::Instance->SetExtents(width, height);
		EventDispatcher::Get().Post(FramebufferSizeEvent(width, height));
	}

	static void WindowFocusCallback(GLFWwindow* window, int focused)
	{
		EventDispatcher::Get().Post(WindowFocusEvent(focused));
	}

	static void WindowRefreshCallback(GLFWwindow* window)
	{
		EventDispatcher::Get().Post(WindowRefreshEvent());
	}

	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
	{
		EventDispatcher::Get().Post(MouseButtonEvent(button, action, mods));
	}

	static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
	{
		EventDispatcher::Get().Post(ScrollEvent(xoffset, yoffset));
	}

	static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
	{
		EventDispatcher::Get().Post(CursorPositionEvent(xpos, ypos));
	}

	static void CursorEnterCallback(GLFWwindow* window, int entered)
	{
		EventDispatcher::Get().Post(CursorEnterEvent(entered));
	}

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		EventDispatcher::Get().Post(KeyEvent(key, scancode, action, mods));
	}

	void Window::Initialize(const WindowSpecification&& spec)
	{
		m_Spec = spec;
	}

	void Window::DispatchMainEm(void* fp)
	{
		std::function<void()>* func = (std::function<void()>*)fp;
		(*func)();
	}

	void Window::DispatchMainLoop(const std::function<void()>& mainLoop)
	{
#ifdef EMSCRIPTEN
		std::function<void()> mainLoopEm = [&]()
		{
			glfwPollEvents();
			mainLoop();
		};
		emscripten_set_main_loop_arg(DispatchMainEm, &mainLoopEm, 0, 1);
#else
		while (!glfwWindowShouldClose(m_Window))
		{
			glfwPollEvents();
			mainLoop();
		}
#endif
	}

	GLFWwindow* Window::GetHandle()
	{
		return m_Window;
	}

	GLFWwindow* Window::GetWorkerHandle()
	{
		return m_WorkerWindow;
	}

	void Window::SetTitle(const std::string& title)
	{
		m_Spec.Title = title;
		glfwSetWindowTitle(m_Window, title.c_str());
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
		if (m_WorkerWindow)
		{
			glfwDestroyWindow(m_WorkerWindow);
		}

		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}

	void Window::AttachEventCallbacks()
	{
		glfwSetWindowCloseCallback(m_Window, WindowCloseCallback);
		glfwSetWindowSizeCallback(m_Window, WindowSizeCallback);
		glfwSetWindowPosCallback(m_Window, WindowPositionCallback);
		glfwSetFramebufferSizeCallback(m_Window, FramebufferSizeCallback);
		glfwSetWindowFocusCallback(m_Window, WindowFocusCallback);
		glfwSetWindowRefreshCallback(m_Window, WindowRefreshCallback);
		glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
		glfwSetScrollCallback(m_Window, ScrollCallback);
		glfwSetCursorPosCallback(m_Window, CursorPosCallback);
		glfwSetCursorEnterCallback(m_Window, CursorEnterCallback);
		glfwSetKeyCallback(m_Window, KeyCallback);

		// Set the initial window position.
		glfwGetWindowPos(m_Window, &m_Position.x, &m_Position.y);
	}
}