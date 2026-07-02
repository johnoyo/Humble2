#include "MacOSPlatformManager.h"

#include "MacOSUtils.h"

namespace HBL2
{
    void MacOSPlatformManager::Initialize()
    {
        m_AppDataDirectory = MacOSUtils::GetAppSupportDir();
        m_ResourcesDirectory = MacOSUtils::GetResourcesDir();
        m_ExecutableDirectory = MacOSUtils::GetExecutableDir();
    }

    void MacOSPlatformManager::Shutdown()
    {
        
    }
}
