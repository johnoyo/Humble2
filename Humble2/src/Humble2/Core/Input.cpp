#include "Input.h"

namespace HBL2
{
	// Define the static member
	Input* Input::s_Instance = nullptr;

	// Define the singleton method
	Input& Input::Get() {
		if (!s_Instance)
		{
			s_Instance = new Input;
		}
		return *s_Instance;
	}

	void Input::Initialize()
	{
		HBL2_CORE_ASSERT(s_Instance == nullptr, "Input::s_Instance is not null! Input::Initialize has been called twice.");
		s_Instance = new Input;

		Get().m_Window = Window::Instance->GetHandle();
	}

	void Input::ShutDown()
	{
		HBL2_CORE_ASSERT(s_Instance != nullptr, "Input::s_Instance is null!");

		delete s_Instance;
		s_Instance = nullptr;
	}

	int Input::IGetKeyDown(int keyCode, int mode)
	{
		int result = 0;

		if (m_Window != nullptr)
		{
			if (keyCode >= 0 && keyCode <= 7)
			{
				result = (glfwGetMouseButton(m_Window, keyCode) == mode);
			}
			else
			{
				result = (glfwGetKey(m_Window, keyCode) == mode);
			}
		}
		else
		{
			HBL2_CORE_ERROR("Window is null!");
		}

		return result;
	}

	int Input::IGetKeyPress(int keyCode)
	{
		int result = 0;

		if (CheckState(keyCode) == GLFW_PRESS && m_LastPressedState[keyCode] == GLFW_RELEASE)
		{
			result = Get().IGetKeyDown(keyCode, GLFW_PRESS);
		}

		m_LastPressedState[keyCode] = CheckState(keyCode);

		return result;
	}

	int Input::IGetKeyRelease(int keyCode)
	{
		int result = 0;

		if (CheckState(keyCode) == GLFW_RELEASE && m_LastReleasedState[keyCode] == GLFW_PRESS)
		{
			result = Get().IGetKeyDown(keyCode, GLFW_RELEASE);
		}

		m_LastReleasedState[keyCode] = CheckState(keyCode);

		return result;
	}

	glm::vec2& Input::IGetMousePosition()
	{
		if (m_Window != nullptr)
		{
			double x, y;
			glfwGetCursorPos(m_Window, &x, &y);
			m_MousePosition.x = (float)x;
			m_MousePosition.y = (float)y;
			return m_MousePosition;
		}
		else
		{
			HBL2_CORE_ERROR("Window is null!");
		}

		return m_MousePosition;
	}


	int Input::CheckState(int keyCode)
	{
		if (m_Window != nullptr)
		{
			if (keyCode >= 0 && keyCode <= 11)
			{
				return glfwGetMouseButton(m_Window, keyCode);
			}
			else
			{
				return glfwGetKey(m_Window, keyCode);
			}
		}
		else
		{
			HBL2_CORE_ERROR("Window is null!");
		}

		return 0;
	}
}