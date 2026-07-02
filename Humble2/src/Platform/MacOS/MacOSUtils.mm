#include "MacOSUtils.h"

#ifdef HBL2_PLATFORM_MACOS
    #import <QuartzCore/CAMetalLayer.h>
    #import <Metal/Metal.h>
    #import <Foundation/Foundation.h>
#endif

namespace HBL2::MacOSUtils
{
    std::string GetAppSupportDir()
    {
#ifdef HBL2_PLATFORM_MACOS
        NSArray* paths = NSSearchPathForDirectoriesInDomains(
            NSApplicationSupportDirectory,
            NSUserDomainMask,
            YES
        );

        NSString* base = [paths firstObject];
        NSString* appDir = [base stringByAppendingPathComponent:@"Humble"];

        [[NSFileManager defaultManager] createDirectoryAtPath:appDir
            withIntermediateDirectories:YES
            attributes:nil
            error:nil];

        return std::string([appDir UTF8String]);
#else
        return  "";
#endif
    }

    std::string GetExecutableDir()
    {
#ifdef HBL2_PLATFORM_MACOS
        NSString* executablePath = [[NSBundle mainBundle] executablePath];
        return std::string([executablePath UTF8String]);
#else
        return  "";
#endif
    }

    std::string GetResourcesDir()
    {
#ifdef HBL2_PLATFORM_MACOS
        NSBundle *bundle = [NSBundle mainBundle];
        NSString *resources = [bundle resourcePath];
        
        return std::string([resources UTF8String]);
#else
        return  "";
#endif
    }
}
