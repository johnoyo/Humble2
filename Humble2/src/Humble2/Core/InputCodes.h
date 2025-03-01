#pragma once

#include "Humble2API.h"

namespace HBL2
{
    enum class HBL2_API KeyCode
    {
        // Mouse Buttons
        MouseLeft = 0, MouseRight, MouseMiddle,
        MouseButton4, MouseButton5, MouseButton6,
        MouseButton7, MouseButton8,

        // Letters
        A = 65, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        // Numbers
        Num0 = 48, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

        // Function Keys
        Escape = 256, Enter, Tab, Backspace, Insert, Delete, Right, Left,
        Down, Up, PageUp, PageDown, Home, End, CapsLock = 280, ScrollLock,
        NumLock, PrintScreen, Pause, F1 = 290, F2, F3, F4, F5, F6, F7, F8,
        F9, F10, F11, F12,

        // Modifiers
        LeftShift = 340, LeftControl, LeftAlt, LeftSuper, RightShift,
        RightControl, RightAlt, RightSuper,

        // Special
        Space = 32, Apostrophe = 39, Comma = 44, Minus, Period, Slash,
        Semicolon = 59, Equal = 61, LeftBracket = 91, Backslash, RightBracket,
        GraveAccent = 96
    };

    enum class HBL2_API GamepadButton
    {
        A = 0, B, X, Y,
        LeftBumper, RightBumper,
        Back, Start, Guide,
        LeftThumb, RightThumb,
        DPadUp, DPadRight, DPadDown, DPadLeft,
        Last = DPadLeft
    };

    enum class HBL2_API GamepadAxis
    {
        LeftX = 0, LeftY, RightX, RightY, LeftTrigger, RightTrigger,
        Last = RightTrigger
    };
}
