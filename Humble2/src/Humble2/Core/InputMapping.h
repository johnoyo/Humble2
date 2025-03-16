#pragma once

#include "InputCodes.h"
#include "Input.h"

#include <unordered_map>
#include <string>

namespace HBL2
{
    /**
     * @struct InputBinding
     * @brief Represents a mapping between keyboard and gamepad inputs for a game action.
     *
     * This structure stores both keyboard and gamepad input options that can trigger the same action.
     */
    struct HBL2_API InputBinding
    {
        KeyCode KeyboardKey;
        GamepadButton GamepadKey;
    };

    /**
     * @class InputMapping
     * @brief Manages input action bindings for keyboard and gamepad controls.
     *
     * This class provides functionality to set up, modify, and check the status of input bindings
     * that map abstract actions to concrete input methods (keyboard keys or gamepad buttons).
     */
    class HBL2_API InputMapping
    {
    public:
        /**
         * @brief Initializes the default input bindings for all actions.
         */
        static void SetDefaultBindings();

        /**
         * @brief Rebinds a named action to a new keyboard key.
         *
         * @param action The name of the action to rebind
         * @param newKey The new keyboard key to assign to the action
         */
        static void RebindAction(const std::string& action, KeyCode newKey);

        /**
         * @brief Rebinds a named action to a new gamepad button.
         *
         * @param action The name of the action to rebind
         * @param newButton The new gamepad button to assign to the action
         */
        static void RebindAction(const std::string& action, GamepadButton newButton);

        /**
         * @brief Checks if the input associated with the specified action is currently pressed.
         *
         * @param action The name of the action to check
         * @return true if the action's input is currently pressed, false otherwise
         */
        static bool IsActionPressed(const std::string& action);

        /**
         * @brief Checks if the input associated with the specified action was pressed this frame.
         *
         * @param action The name of the action to check
         * @return true if the action's input was pressed this frame, false otherwise
         */
        static bool IsActionDown(const std::string& action);

        /**
         * @brief Checks if the input associated with the specified action was released this frame.
         *
         * @param action The name of the action to check
         * @return true if the action's input was released this frame, false otherwise
         */
        static bool IsActionReleased(const std::string& action);

    private:
        static std::unordered_map<std::string, InputBinding> s_Bindings;
    };
}
