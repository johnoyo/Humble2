#pragma once

#ifdef HBL2_PLATFORM_MACOS

#include <string>

struct GLFWwindow;

namespace HBL2
{
    void EnsureMetalLayerBacking(GLFWwindow* window);
    void DebugMetalLayer(GLFWwindow* window);
    void DebugMetalLayerMid(GLFWwindow* window);
    std::string GetAppSupportDir();
    std::string GetExecutableDir();
    std::string GetResourcesDir();
}

#endif
