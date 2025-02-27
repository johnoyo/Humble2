#include "InputMapping.h"

namespace HBL2
{
    std::unordered_map<std::string, InputBinding> InputMapping::s_Bindings;

    void InputMapping::SetDefaultBindings()
    {
        s_Bindings["MoveForward"] = { KeyCode::W, GamepadButton::DPadUp };
        s_Bindings["MoveBackward"] = { KeyCode::S, GamepadButton::DPadDown };
        s_Bindings["MoveLeft"] = { KeyCode::A, GamepadButton::DPadLeft };
        s_Bindings["MoveRight"] = { KeyCode::D, GamepadButton::DPadRight };
        s_Bindings["Jump"] = { KeyCode::Space, GamepadButton::A };
        s_Bindings["Attack"] = { KeyCode::LeftControl, GamepadButton::X };
        s_Bindings["Sprint"] = { KeyCode::LeftShift, GamepadButton::LeftBumper };
    }

    void InputMapping::RebindAction(const std::string& action, KeyCode newKey)
    {
        if (s_Bindings.find(action) != s_Bindings.end())
        {
            s_Bindings[action].KeyboardKey = newKey;
        }
    }

    void InputMapping::RebindAction(const std::string& action, GamepadButton newButton)
    {
        if (s_Bindings.find(action) != s_Bindings.end())
        {
            s_Bindings[action].GamepadKey = newButton;
        }
    }

    bool InputMapping::IsActionPressed(const std::string& action)
    {
        auto it = s_Bindings.find(action);
        if (it == s_Bindings.end())
        {
            return false;
        }

        return Input::GetKeyPress(it->second.KeyboardKey) || Input::GetGamepadButtonPress(it->second.GamepadKey);
    }

    bool InputMapping::IsActionDown(const std::string& action)
    {
        auto it = s_Bindings.find(action);
        if (it == s_Bindings.end())
        {
            return false;
        }

        return Input::GetKeyDown(it->second.KeyboardKey) || Input::GetGamepadButtonDown(it->second.GamepadKey);
    }

    bool InputMapping::IsActionReleased(const std::string& action)
    {
        auto it = s_Bindings.find(action);
        if (it == s_Bindings.end())
        {
            return false;
        }

        return Input::GetKeyRelease(it->second.KeyboardKey);
    }
}
