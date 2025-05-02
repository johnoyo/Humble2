#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanDevice.h"

#include "Platform\Vulkan\VulkanCommon.h"

namespace HBL2
{
	struct VulkanBindGroupLayout
	{
		VulkanBindGroupLayout() = default;
		VulkanBindGroupLayout(const BindGroupLayoutDescriptor&& desc);

		void Destroy();

		const char* DebugName = "";
		VkDescriptorSetLayout DescriptorSetLayout = VK_NULL_HANDLE;
		std::vector<BindGroupLayoutDescriptor::BufferBinding> BufferBindings;
		std::vector<BindGroupLayoutDescriptor::TextureBinding> TextureBindings;
	};
}