#pragma once

#include "Utilities/PlatformManager.h"

namespace HBL2
{
    class WindowsPlatformManager final : public PlatformManager
    {
    public:
        ~WindowsPlatformManager() = default;
        
        virtual void Initialize() override;
        virtual void Shutdown() override;
    };
}
