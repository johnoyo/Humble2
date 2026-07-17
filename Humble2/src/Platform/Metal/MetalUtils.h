#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

void ConnectWindowWithMetal(GLFWwindow* window, CA::MetalLayer* metalLayer);
