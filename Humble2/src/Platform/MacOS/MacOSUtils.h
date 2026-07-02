#pragma once

#ifdef HBL2_PLATFORM_MACOS

#include <string>

namespace HBL2::MacOSUtils
{
    std::string GetAppSupportDir();
    std::string GetExecutableDir();
    std::string GetResourcesDir();
}

#endif
