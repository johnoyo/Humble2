#include "PlatformManager.h"

namespace HBL2
{
    PlatformManager* PlatformManager::Instance = nullptr;

    const std::string& PlatformManager::GetAppDataDirectory()
    {
        return m_AppDataDirectory;
    }

    const std::string& PlatformManager::GetResourcesDirectory()
    {
        return m_ResourcesDirectory;
    }

    const std::string& PlatformManager::GetExecutableDirectory()
    {
        return m_ExecutableDirectory;
    }
}
