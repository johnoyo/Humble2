#pragma once

#include "Utilities/PlatformManager.h"

namespace HBL2
{
    class MacOSPlatformManager final : public PlatformManager
    {
    public:
        ~MacOSPlatformManager() = default;
        
        virtual void Initialize() override;
        virtual void Shutdown() override;
    };
}
