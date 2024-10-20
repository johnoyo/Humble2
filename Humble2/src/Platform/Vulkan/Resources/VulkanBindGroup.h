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

		void Destroy()
		{
		}

		const char* DebugName = "";
		VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;

		std::vector<BindGroupDescriptor::BufferEntry> Buffers;
		std::vector<Handle<Texture>> Textures;
		Handle<BindGroupLayout> BindGroupLayout;
	};
}