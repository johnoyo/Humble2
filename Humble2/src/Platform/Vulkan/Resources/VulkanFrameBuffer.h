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
		VulkanFrameBuffer(const FrameBufferDescriptor&& desc) 
			: DebugName(desc.debugName), Width(desc.width), Height(desc.height), ColorTargets(desc.colorTargets), DepthTarget(desc.depthTarget), RenderPass(desc.renderPass)
		{
			Create();
		}

		void Create();

		void Resize(uint32_t width, uint32_t height)
		{
			if (Width == width && Height == height)
			{
				return;
			}

			Width = width;
			Height = height;

			Destroy();

			Create();
		}

		void Destroy()
		{
			VulkanDevice* device = (VulkanDevice*)Device::Instance;
			vkDestroyFramebuffer(device->Get(), FrameBuffer, nullptr);
		}

		const char* DebugName = "";
		VkFramebuffer FrameBuffer = VK_NULL_HANDLE;
		uint32_t Width = 0;
		uint32_t Height = 0;
		std::vector<Handle<Texture>> ColorTargets;
		Handle<Texture> DepthTarget;
		Handle<RenderPass> RenderPass;
	};
}