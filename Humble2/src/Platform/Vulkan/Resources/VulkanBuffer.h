#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "vulkan\vulkan.h"
#include "vma\vk_mem_alloc.h"

#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>

namespace HBL2
{
	struct VulkanBuffer
	{
		VulkanBuffer() = default;
		VulkanBuffer(const BufferDescriptor&& desc)
		{
			DebugName = desc.debugName;
		}

		const char* DebugName = "";
		VkBuffer Buffer;
		VmaAllocation Allocation;
	};
}