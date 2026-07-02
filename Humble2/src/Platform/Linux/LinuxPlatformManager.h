#pragma once

#include "Utilities/PlatformManager.h"

namespace HBL2
{
    class LinuxPlatformManager final : public PlatformManager
    {
    public:
        ~LinuxPlatformManager() = default;
        
        virtual void Initialize() override;
        virtual void Shutdown() override;
    };
}
