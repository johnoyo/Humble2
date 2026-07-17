#include "MetalUtils.h"

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>

void ConnectWindowWithMetal(GLFWwindow* window, CA::MetalLayer* metalLayer)
{
    // Connect window with metal.
    // metal-cpp's NS::Object-derived pointers are the underlying Objective-C
    // `id` reinterpreted, so a __bridge cast to CALayer* is exactly correct
    // here, this is the standard way to hand a metal-cpp CAMetalLayer to
    // an AppKit NSView.
    NSWindow* nsWindow = glfwGetCocoaWindow(window);
    nsWindow.contentView.layer = (__bridge CALayer*)metalLayer;
    nsWindow.contentView.wantsLayer = YES;
}
