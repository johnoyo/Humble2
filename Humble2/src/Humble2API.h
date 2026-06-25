#pragma once

#ifdef HBL2_PLATFORM_WINDOWS
    #ifdef HBL2_BUILD_DLL
        #define HBL2_API __declspec(dllexport)
    #else
        #define HBL2_API __declspec(dllimport)
    #endif
#elif HBL2_PLATFORM_MACOS || HBL2_PLATFORM_LINUX
    #ifdef HBL2_BUILD_DLL
        #define HBL2_API __attribute__((visibility("default")))
    #else
        #define HBL2_API
    #endif
#endif
