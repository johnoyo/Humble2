#include "PlatformManager.h"

#include "Platform/Windows/WindowsPlatformManager.h"
#include "Platform/MacOS/MacOSPlatformManager.h"
#include "Platform/Linux/LinuxPlatformManager.h"

namespace HBL2
{
    PlatformManager* PlatformManager::Instance = nullptr;

    PlatformManager* PlatformManager::Create()
    {
#ifdef HBL2_PLATFORM_WINDOWS
        return new WindowsPlatformManager;
#elif HBL2_PLATFORM_MACOS
        return new MacOSPlatformManager;
#elif HBL2_PLATFORM_LINUX
        return new LinuxPlatformManager;
#endif
    }

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
