#pragma once

#include <Base.h>

#include "vulkan\vulkan.h"
#include "vma\vk_mem_alloc.h"

#include <string>
#include <format>

#define VK_VALIDATE(result, vkFunction) HBL2_CORE_ASSERT(result == VK_SUCCESS, std::format("Vulkan function: {}, failed!", vkFunction));
