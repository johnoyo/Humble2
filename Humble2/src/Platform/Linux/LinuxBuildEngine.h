#pragma once

#include "Script/BuildEngine.h"

namespace HBL2
{
    class LinuxBuildEngine final : public BuildEngine
    {
    public:
        virtual bool Build() override;
        virtual bool RunRuntime(Configuration configuration) override;
        virtual bool BuildRuntime(Configuration configuration) override;
        virtual const std::filesystem::path GetUnityBuildPath(Configuration config) override;
        
    private:
        const std::filesystem::path GetUnityBuildBasePath(Configuration config) const;
        void Combine();
        
    private:
        std::string m_CurrentSoName;
        uint32_t m_SoLoadCount = 0;
    };
}
