#pragma once

#include "Base.h"
#include "Resources/TypeDescriptors.h"

#include "Platform/Vulkan/VulkanDevice.h"
#include "Platform/Vulkan/VulkanRenderer.h"

#include "Platform/Vulkan/VulkanCommon.h"

#include "VulkanRenderPassLayout.h"

#include <string>
#include <stdint.h>

namespace HBL2
{
	struct VulkanRenderPass
	{
		VulkanRenderPass() = default;
		VulkanRenderPass(const RenderPassDescriptor&& desc);

        void CreateFrameBuffer(const FrameBufferDescriptor&& desc);
        
		void Destroy()
		{
			VulkanDevice* device = (VulkanDevice*)Device::Instance;
			vkDestroyRenderPass(device->Get(), RenderPass, nullptr);
            vkDestroyFramebuffer(device->Get(), FrameBuffer, nullptr);
		}

		const char* DebugName = "";
		VkRenderPass RenderPass = VK_NULL_HANDLE;
		std::vector<bool> ColorClearValues;
		glm::vec4 ClearColor = {};

		bool DepthClearValue = false;
		float ClearDepth = 1.0f;

		bool StencilClearValue = false;
		uint32_t ClearStencil = 0;
        
        // FrameBuffer data.
        VkFramebuffer FrameBuffer = VK_NULL_HANDLE;
        uint32_t Width = 0;
        uint32_t Height = 0;
        std::vector<Handle<Texture>> ColorTargets;
        Handle<Texture> DepthTarget;
	};
}
