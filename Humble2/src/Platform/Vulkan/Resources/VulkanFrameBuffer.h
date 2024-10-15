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

		const char* DebugName = "";
		VkFramebuffer FrameBuffer;
	};
}