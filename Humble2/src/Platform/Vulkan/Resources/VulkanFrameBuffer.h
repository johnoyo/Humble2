#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanDevice.h"

#include "Platform\Vulkan\VulkanCommon.h"

#include "VulkanTexture.h"
#include "VulkanRenderPass.h"

namespace HBL2
{
	struct VulkanFrameBuffer
	{
		VulkanFrameBuffer() = default;
		VulkanFrameBuffer(const FrameBufferDescriptor&& desc);

		void Resize(uint32_t width, uint32_t height)
		{

		}

		void Destroy()
		{
			VulkanDevice* device = (VulkanDevice*)Device::Instance;

			vkDestroyFramebuffer(device->Get(), FrameBuffer, nullptr);
		}

		const char* DebugName = "";
		VkFramebuffer FrameBuffer = VK_NULL_HANDLE;
	};
}