#include "VulkanWindow.h"

#include <GLFW/glfw3.h>

namespace HBL2
{
	static void ErrorCallback(int error, const char* description)
	{
		HBL2_CORE_ERROR("GLFW Error {}: {}", error, description);
	}

	void VulkanWindow::Create()
	{
		EventDispatcher::Get().Register<WindowSizeEvent>([](const WindowSizeEvent& e)
		{
			// TODO
		});

		if (!glfwInit())
		{
			HBL2_CORE_FATAL("Error initializing window!");
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
			HBL2_CORE_FATAL("Error creating window!");
			glfwTerminate();
			exit(-1);
		}

		AttachEventCallbacks();
	}

	void VulkanWindow::Setup()
	{
		// No extra context windows for worker threads in vulkan,
		for (int i = 0; i < MAX_WORKERS; ++i)
		{
			m_WorkerWindows[i] = nullptr;
		}
	}
}
