#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanDevice.h"
#include "Platform\Vulkan\VulkanRenderer.h"

#include "Platform\Vulkan\VulkanCommon.h"

#include "VulkanRenderPassLayout.h"

#include <string>
#include <stdint.h>

namespace HBL2
{
	struct VulkanRenderPass
	{
		VulkanRenderPass() = default;
		VulkanRenderPass(const RenderPassDescriptor&& desc);

		void Destroy()
		{
			VulkanDevice* device = (VulkanDevice*)Device::Instance;
			vkDestroyRenderPass(device->Get(), RenderPass, nullptr);
		}

		const char* DebugName = "";
		VkRenderPass RenderPass = VK_NULL_HANDLE;
		std::vector<bool> ColorClearValues;
		bool DepthClearValue = false;
	};
}