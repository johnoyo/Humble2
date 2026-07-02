#include "MacOSUtils.h"

#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

namespace HBL2::MacOSUtils
{
    std::string GetAppSupportDir()
    {
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
    }

    std::string GetExecutableDir()
    {
        NSString* executablePath = [[NSBundle mainBundle] executablePath];
        return std::string([executablePath UTF8String]);
    }

    std::string GetResourcesDir()
    {
        NSBundle *bundle = [NSBundle mainBundle];
        NSString *resources = [bundle resourcePath];
        
        return std::string([resources UTF8String]);
    }
}
