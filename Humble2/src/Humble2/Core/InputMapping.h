#pragma once

#include "InputCodes.h"
#include "Input.h"

#include <unordered_map>
#include <string>

namespace HBL2
{
    struct HBL2_API InputBinding
    {
        KeyCode KeyboardKey;
        GamepadButton GamepadKey;
    };

    class HBL2_API InputMapping
    {
    public:
        static void SetDefaultBindings();
        static void RebindAction(const std::string& action, KeyCode newKey);
        static void RebindAction(const std::string& action, GamepadButton newButton);

        static bool IsActionPressed(const std::string& action);
        static bool IsActionDown(const std::string& action);
        static bool IsActionReleased(const std::string& action);

    private:
        static std::unordered_map<std::string, InputBinding> s_Bindings;
    };
}
