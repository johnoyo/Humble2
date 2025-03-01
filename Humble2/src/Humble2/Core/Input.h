#pragma once

#include "Base.h"
#include "Window.h"
#include "InputCodes.h"

#include <GLFW/glfw3.h>

namespace HBL2
{
    class HBL2_API Input
    {
    public:
        Input(const Input&) = delete;

        static Input& Get();

        static void Initialize();
        static void ShutDown();

        static const glm::vec2& GetMousePosition() { return Get().IGetMousePosition(); }

        static bool GetKeyUp(KeyCode key) { return Get().IGetKeyDown((int)key, GLFW_RELEASE); }
        static bool GetKeyDown(KeyCode key) { return Get().IGetKeyDown((int)key, GLFW_PRESS); }
        static bool GetKeyPress(KeyCode key) { return Get().IGetKeyPress((int)key); }
        static bool GetKeyRelease(KeyCode key) { return Get().IGetKeyRelease((int)key); }

        static bool IsGamepadConnected(int gamepad = 0);
        static bool GetGamepadButtonDown(GamepadButton button, int gamepad = 0);
        static bool GetGamepadButtonPress(GamepadButton button, int gamepad = 0);
        static float GetGamepadAxis(GamepadAxis axis, int gamepad = 0);

    private:
        bool IGetKeyDown(int keyCode, int mode);
        bool IGetKeyPress(int keyCode);
        bool IGetKeyRelease(int keyCode);
        const glm::vec2& IGetMousePosition();

        Input() {}

        int m_LastReleasedState[512] = { GLFW_RELEASE };
        int m_LastPressedState[512] = { GLFW_PRESS };
        bool m_LastGamepadState[16][15] = {};
        glm::vec2 m_MousePosition = {};

        GLFWwindow* m_Window = nullptr;

        int CheckState(int keyCode);

        static Input* s_Instance;
    };
}