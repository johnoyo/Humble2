#include "OpenGLWindow.h"
#include "Core/Input.h"

namespace HBL2
{
	void WindowResizeCallback(GLFWwindow* window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void OpenGLWindow::Create()
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

		m_Spec.RefreshRate = (float)mode->refreshRate;

		if (m_Spec.FullScreen)
		{
			m_Window = glfwCreateWindow(mode->width, mode->height, m_Spec.Title.c_str(), glfwGetPrimaryMonitor(), NULL);
		}
		else
		{
			m_Window = glfwCreateWindow((int)m_Spec.Width, (int)m_Spec.Height, m_Spec.Title.c_str(), NULL, NULL);
		}

		if (!m_Window)
		{
			std::cout << "Error creating window!\n";
			glfwTerminate();
			exit(-1);
		}

		if (m_Spec.VerticalSync)
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

		Input::SetWindow(m_Window);
	}

	void OpenGLWindow::Present()
	{
		glfwSwapBuffers(m_Window);
	}
}
