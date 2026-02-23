#include "Input.h"

#include <GLFW/glfw3.h>

namespace HBL2
{
    Input* Input::s_Instance = nullptr;

    Input& Input::Get()
    {
        if (!s_Instance)
        {
            s_Instance = new Input;
        }

        return *s_Instance;
    }

    void Input::Initialize()
    {
        HBL2_CORE_ASSERT(s_Instance == nullptr, "Input::Initialize called twice.");
        s_Instance = new Input;
        s_Instance->m_Window = Window::Instance->GetHandle();
    }

    void Input::ShutDown()
    {
        HBL2_CORE_ASSERT(s_Instance != nullptr, "Input::ShutDown called without initialization.");
        delete s_Instance;
        s_Instance = nullptr;
    }

    bool Input::IsGamepadConnected(int gamepad)
    {
        return glfwJoystickPresent(gamepad) == GLFW_TRUE && glfwJoystickIsGamepad(gamepad) == GLFW_TRUE;
    }

    bool Input::GetGamepadButtonDown(GamepadButton button, int gamepad)
    {
        if (!IsGamepadConnected(gamepad))
        {
            return false;
        }

        GLFWgamepadstate state;
        if (glfwGetGamepadState(gamepad, &state))
        {
            return state.buttons[(int)button] == GLFW_PRESS;
        }

        return false;
    }

    bool Input::GetGamepadButtonPress(GamepadButton button, int gamepad)
    {
        if (!IsGamepadConnected(gamepad))
        {
            return false;
        }

        GLFWgamepadstate state;
        if (glfwGetGamepadState(gamepad, &state))
        {
            bool pressed = (state.buttons[(int)button] == GLFW_PRESS);
            bool wasPressed = s_Instance->m_LastGamepadState[gamepad][(int)button];

            s_Instance->m_LastGamepadState[gamepad][(int)button] = pressed;
            return pressed && !wasPressed;
        }

        return false;
    }

    float Input::GetGamepadAxis(GamepadAxis axis, int gamepad)
    {
        if (!IsGamepadConnected(gamepad))
        {
            return 0.0f;
        }

        GLFWgamepadstate state;
        if (glfwGetGamepadState(gamepad, &state))
        {
            return state.axes[(int)axis];
        }
        return 0.0f;
    }

    bool Input::IGetKeyDown(int keyCode, int mode)
    {
        if (!m_Window)
        {
            HBL2_CORE_ERROR("Window is null!");
            return false;
        }

        bool result = false;

        if (keyCode >= 0 && keyCode <= 7)
        {
            result = (glfwGetMouseButton(m_Window, keyCode) == mode);
        }
        else
        {
            result = (glfwGetKey(m_Window, keyCode) == mode);
        }

        return result;
    }

    bool Input::IGetKeyPress(int keyCode)
    {
        int current = CheckState(keyCode);
        bool pressed = (current == GLFW_PRESS) && (m_LastPressedState[keyCode] == GLFW_RELEASE);

        m_LastPressedState[keyCode] = current;
        return pressed;
    }

    bool Input::IGetKeyRelease(int keyCode)
    {
        int current = CheckState(keyCode);
        bool released = (current == GLFW_RELEASE) && (m_LastReleasedState[keyCode] == GLFW_PRESS);

        m_LastReleasedState[keyCode] = current;
        return released;
    }

    const glm::vec2& Input::IGetMousePosition()
    {
        if (!m_Window)
        {
            HBL2_CORE_ERROR("Window is null!");
            return m_MousePosition;
        }

        double x, y;
        glfwGetCursorPos(m_Window, &x, &y);
        m_MousePosition.x = (float)x;
        m_MousePosition.y = (float)y;

        return m_MousePosition;
    }

    int Input::CheckState(int keyCode)
    {
        if (!m_Window)
        {
            HBL2_CORE_ERROR("Window is null!");
            return 0;
        }

        return glfwGetKey(m_Window, keyCode);
    }
}