#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanCommon.h"

namespace HBL2
{
	struct VulkanBindGroup
	{
		VulkanBindGroup() = default;
		VulkanBindGroup(const BindGroupDescriptor&& desc)
		{
			DebugName = desc.debugName;
		}

		const char* DebugName = "";
		VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
		VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;
	};
}