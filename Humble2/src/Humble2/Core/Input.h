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

        /**
         * @brief Returns the current mouse position.
         *
         * @return The current mouse position.
         */
        static const glm::vec2& GetMousePosition() { return Get().IGetMousePosition(); }

        /**
         * @brief Checks if the specified key was released during the current frame.
         *
         * @param key The keyboard key code to check for release.
         * @return True if the key was released during this frame, false otherwise.
         */
        static bool GetKeyUp(KeyCode key) { return Get().IGetKeyDown((int)key, GLFW_RELEASE); }

        /**
         * @brief Checks if the specified key was pressed during the current frame.
         *
         * @param key The keyboard key code to check for press.
         * @return True if the key was pressed during this frame, false otherwise.
         */
        static bool GetKeyDown(KeyCode key) { return Get().IGetKeyDown((int)key, GLFW_PRESS); }

        /**
         * @brief Checks if the specified key was pressed during the current frame.
         *
         * @param key The keyboard key code to check for press.
         * @return True if the key was pressed during this frame, false otherwise.
         */
        static bool GetKeyPress(KeyCode key) { return Get().IGetKeyPress((int)key); }

        /**
         * @brief Checks if the specified key was released since the last frame.
         *
         * @param key The keyboard key code to check for release state.
         * @return True if the key was released since the last frame, false otherwise.
         */
        static bool GetKeyRelease(KeyCode key) { return Get().IGetKeyRelease((int)key); }

        /**
         * @brief Returns whether the specified gamepad is connected or not.
         *
         * @param gamepad The gamepad index.
         * @return True if the specified gamepad is connected, false otherwise.
         */
        static bool IsGamepadConnected(int gamepad = 0);

        /**
         * @brief Checks if the specified gamepad button was pressed during the current frame.
         *
         * @param button The gamepad button to check for press.
         * @param gamepad The index of the gamepad to check (defaults to 0 for the first connected gamepad).
         * @return True if the specified button was pressed during this frame, false otherwise.
         */
        static bool GetGamepadButtonDown(GamepadButton button, int gamepad = 0);

        /**
         * @brief Checks if the specified gamepad button is currently being pressed.
         *
         * @param button The gamepad button to check for pressed state.
         * @param gamepad The index of the gamepad to check (defaults to 0 for the first connected gamepad).
         * @return True if the specified button is currently pressed, false otherwise.
         */
        static bool GetGamepadButtonPress(GamepadButton button, int gamepad = 0);

        /**
         * @brief Returns the current value of the specified gamepad axis.
         *
         * @param axis The gamepad axis to get the value from.
         * @param gamepad The index of the gamepad to check (defaults to 0 for the first connected gamepad).
         * @return The current value of the specified axis, typically in the range of -1.0 to 1.0.
         */
        static float GetGamepadAxis(GamepadAxis axis, int gamepad = 0);

    private:
        bool IGetKeyDown(int keyCode, int mode);
        bool IGetKeyPress(int keyCode);
        bool IGetKeyRelease(int keyCode);
        const glm::vec2& IGetMousePosition();

        Input() {}

        int m_LastReleasedState[512] = { GLFW_RELEASE };
        int m_LastPressedState[512] = { GLFW_RELEASE };
        bool m_LastGamepadState[16][15] = {};
        glm::vec2 m_MousePosition = {};

        GLFWwindow* m_Window = nullptr;

        int CheckState(int keyCode);

        static Input* s_Instance;
    };
}