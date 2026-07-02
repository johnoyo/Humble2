#include "MacOSPlatformManager.h"

#include "MacOSUtils.h"

namespace HBL2
{
    void MacOSPlatformManager::Initialize()
    {
#ifdef HBL2_PLATFORM_MACOS
        m_AppDataDirectory = MacOSUtils::GetAppSupportDir();
        m_ResourcesDirectory = MacOSUtils::GetResourcesDir();
        m_ExecutableDirectory = MacOSUtils::GetExecutableDir();
#endif
    }

    void MacOSPlatformManager::Shutdown()
    {
        
    }
}
