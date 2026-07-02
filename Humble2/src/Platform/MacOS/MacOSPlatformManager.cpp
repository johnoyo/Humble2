#include "MacOSPlatformManager.h"

#include "Core/Context.h"
#include "MacOSUtils.h"

namespace HBL2
{
    void MacOSPlatformManager::Initialize()
    {
#ifdef HBL2_PLATFORM_MACOS
        if (Context::Mode == Mode::Editor)
        {
            m_AppDataDirectory = MacOSUtils::GetAppSupportDir() + "/Editor";
        }
        else
        {
            m_AppDataDirectory = MacOSUtils::GetAppSupportDir() + "/Runtime";         
        }
        
        m_ResourcesDirectory = MacOSUtils::GetResourcesDir();
        m_ExecutableDirectory = MacOSUtils::GetExecutableDir();
#endif
    }

    void MacOSPlatformManager::Shutdown()
    {
        
    }
}
