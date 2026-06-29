#pragma once

#include "Script/BuildEngine.h"

namespace HBL2
{
    class MacOSBuildEngine final : public BuildEngine
    {
    public:
        virtual bool Build() override;
        virtual bool RunRuntime(Configuration configuration) override;
        virtual bool BuildRuntime(Configuration configuration) override;
    };
}
