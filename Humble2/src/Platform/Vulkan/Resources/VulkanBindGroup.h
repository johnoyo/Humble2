#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanDevice.h"

#include "Platform\Vulkan\VulkanCommon.h"

namespace HBL2
{
	struct VulkanBindGroup
	{
		VulkanBindGroup() = default;
		VulkanBindGroup(const BindGroupDescriptor&& desc);

		void Destroy()
		{
			VulkanDevice* device = (VulkanDevice*)Device::Instance;

			vkDestroyDescriptorSetLayout(device->Get(), DescriptorSetLayout, nullptr);
		}

		const char* DebugName = "";
		VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
		VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;

		std::vector<BindGroupDescriptor::BufferEntry> Buffers;
		std::vector<Handle<Texture>> Textures;
		Handle<BindGroupLayout> BindGroupLayout;
	};
}