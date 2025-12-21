#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanDevice.h"
#include "Platform\Vulkan\VulkanRenderer.h"

#include "Platform\Vulkan\VulkanCommon.h"

namespace HBL2
{
	struct VulkanBindGroup
	{
		VulkanBindGroup() = default;
		VulkanBindGroup(const BindGroupDescriptor&& desc);

		void Update();
		void Destroy();

		const char* DebugName = "";
		VkDescriptorSet DescriptorSet = VK_NULL_HANDLE; // Hot

		std::vector<BindGroupDescriptor::BufferEntry> Buffers; // Hot
		std::vector<Handle<Texture>> Textures;
		Handle<BindGroupLayout> BindGroupLayout;
	};
}