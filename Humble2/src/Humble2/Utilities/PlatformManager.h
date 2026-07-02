#pragma once

#include "Humble2API.h"

#include <string>

namespace HBL2
{
    class HBL2_API PlatformManager
    {
    public:
        static PlatformManager* Instance;

        PlatformManager() = default;
        virtual ~PlatformManager() = default;
        
        virtual void Initialize() = 0;
        virtual void Shutdown() = 0;
        
        const std::string& GetAppDataDirectory();
        const std::string& GetExecutableDirectory();
        const std::string& GetResourcesDirectory();
        
    protected:
        std::string m_AppDataDirectory;
        std::string m_ExecutableDirectory;
        std::string m_ResourcesDirectory;
        std::string m_EmptyDirectory = "";
    };
}
