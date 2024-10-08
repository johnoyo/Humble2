#include "VulkanWindow.h"

namespace HBL2
{
	static void ErrorCallback(int error, const char* description)
	{
		HBL2_CORE_ERROR("GLFW Error {}: {}", error, description);
	}

	void VulkanWindow::Create()
	{
		EventDispatcher::Get().Register("WindowSizeEvent", [](const Event& e)
		{
			const WindowSizeEvent& wse = dynamic_cast<const WindowSizeEvent&>(e);
			// TODO
		});

		if (!glfwInit())
		{
			std::cout << "Error initializing window!\n";
			exit(-1);
		}

		glfwSetErrorCallback(ErrorCallback);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		if (m_Spec.FullScreen)
		{
			const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

			glfwWindowHint(GLFW_RED_BITS, mode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
			glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

			m_Spec.RefreshRate = (float)mode->refreshRate;

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

		AttachEventCallbacks();
	}

	void VulkanWindow::Present()
	{
		// TODO:
	}
}
