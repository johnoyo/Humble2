#pragma once

#include "Core/Window.h"

namespace HBL2
{
    class MetalWindow final : public Window
    {
    public:
        ~MetalWindow() = default;

        virtual void Create() override;
        virtual void Setup() override;
    };
}
