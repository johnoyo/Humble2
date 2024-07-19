#include "Window.h"

#include "Input.h"

namespace HBL2
{
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
		glfwTerminate();
	}
}