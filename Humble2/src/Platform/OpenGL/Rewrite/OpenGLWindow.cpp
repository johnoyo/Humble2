#include "OpenGLWindow.h"
#include "Core/Input.h"

namespace HBL2
{
	static void ErrorCallback(int error, const char* description)
	{
		HBL2_CORE_ERROR("Error {}: {}", error, description);
	}

	void OpenGLWindow::Create()
	{
		EventDispatcher::Get().Register("WindowSizeEvent", [](const Event& e)
		{
			const WindowSizeEvent& wse = dynamic_cast<const WindowSizeEvent&>(e);
			glViewport(0, 0, wse.Width, wse.Height);
		});

		if (!glfwInit())
		{
			std::cout << "Error initializing window!\n";
			exit(-1);
		}

		glfwSetErrorCallback(ErrorCallback);

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

		glfwMakeContextCurrent(m_Window);

		if (m_Spec.VerticalSync)
		{
			glfwSwapInterval(1);
		}
		else
		{
			glfwSwapInterval(0);
		}

#ifndef EMSCRIPTEN
		if (glewInit() != GLEW_OK)
		{
			std::cout << "Error initializing GLEW!\n";
			exit(-1);
		}
#endif
		AttachEventCallbacks();
	}

	void OpenGLWindow::Present()
	{
		glfwSwapBuffers(m_Window);
	}
}
