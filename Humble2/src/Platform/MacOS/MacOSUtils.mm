#include "MacOSUtils.h"

#ifdef HBL2_PLATFORM_MACOS

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

#import <QuartzCore/CAMetalLayer.h>
#import <Metal/Metal.h>
#import <Foundation/Foundation.h>

namespace HBL2
{
    void EnsureMetalLayerBacking(GLFWwindow* window)
    {
        NSWindow* nsWin = glfwGetCocoaWindow(window);
        NSView*   view  = [nsWin contentView];

        if (![view.layer isKindOfClass:[CAMetalLayer class]])
        {
            CAMetalLayer* layer = [CAMetalLayer layer];
            layer.device        = MTLCreateSystemDefaultDevice();
            layer.pixelFormat   = MTLPixelFormatBGRA8Unorm;
            layer.autoresizingMask           = kCALayerWidthSizable | kCALayerHeightSizable;
            layer.needsDisplayOnBoundsChange = YES;

            [view setLayer:layer];
            [view setWantsLayer:YES];
            [view setLayerContentsRedrawPolicy:NSViewLayerContentsRedrawDuringViewResize];
        }

        CAMetalLayer* metal = (CAMetalLayer*)view.layer;

        // Fix 1: set actual pixel drawable size
        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        metal.drawableSize = CGSizeMake(fbW, fbH);

        // Fix 2: match Retina scale factor
        metal.contentsScale = nsWin.screen.backingScaleFactor;
    }

    void DebugMetalLayer(GLFWwindow* window)
    {
        NSWindow* nsWin = glfwGetCocoaWindow(window);
        NSView*   view  = [nsWin contentView];

        NSLog(@"[HBL2] Layer class: %@", NSStringFromClass([view.layer class]));
        NSLog(@"[HBL2] wantsLayer: %d", [view wantsLayer]);
        NSLog(@"[HBL2] isOpaque: %d", [view isOpaque]);
        NSLog(@"[HBL2] contentView: %@", view);

        if ([view.layer isKindOfClass:[CAMetalLayer class]])
        {
            CAMetalLayer* metal = (CAMetalLayer*)view.layer;
            NSLog(@"[HBL2] CAMetalLayer device: %@", metal.device.name);
            NSLog(@"[HBL2] CAMetalLayer pixelFormat: %lu", (unsigned long)metal.pixelFormat);
            NSLog(@"[HBL2] CAMetalLayer drawableSize: %f x %f", metal.drawableSize.width, metal.drawableSize.height);
            NSLog(@"[HBL2] CAMetalLayer contentsScale: %f", metal.contentsScale);
        }
        else
        {
            NSLog(@"[HBL2] ERROR: Layer is NOT CAMetalLayer — surface will not present!");
        }

        // Check framebuffer vs window size
        int winW, winH, fbW, fbH;
        glfwGetWindowSize(window, &winW, &winH);
        glfwGetFramebufferSize(window, &fbW, &fbH);
        NSLog(@"[HBL2] Window size (points): %d x %d", winW, winH);
        NSLog(@"[HBL2] Framebuffer size (pixels): %d x %d", fbW, fbH);
    }

    void DebugMetalLayerMid(GLFWwindow* window)
    {
        NSWindow* nsWin = glfwGetCocoaWindow(window);
        NSView*   view  = [nsWin contentView];
        CAMetalLayer* metal = (CAMetalLayer*)view.layer;

        NSLog(@"[HBL2-MID] drawableSize: %f x %f", metal.drawableSize.width, metal.drawableSize.height);
        NSLog(@"[HBL2-MID] contentsScale: %f", metal.contentsScale);
        NSLog(@"[HBL2-MID] framebufferOnly: %d", metal.framebufferOnly);
        NSLog(@"[HBL2-MID] presentsWithTransaction: %d", metal.presentsWithTransaction);
        NSLog(@"[HBL2-MID] displaySyncEnabled: %d", metal.displaySyncEnabled);
        NSLog(@"[HBL2-MID] isOpaque: %d", metal.opaque);
        NSLog(@"[HBL2-MID] hidden: %d", view.hidden);
        NSLog(@"[HBL2-MID] window visible: %d", [nsWin isVisible]);
        NSLog(@"[HBL2-MID] window screen: %@", [nsWin screen]);
    }

    std::string GetAppSupportDir()
    {
        NSArray* paths = NSSearchPathForDirectoriesInDomains(
            NSApplicationSupportDirectory,
            NSUserDomainMask,
            YES
        );

        NSString* base = [paths firstObject];
        NSString* appDir = [base stringByAppendingPathComponent:@"HumbleApp"];

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

#endif
