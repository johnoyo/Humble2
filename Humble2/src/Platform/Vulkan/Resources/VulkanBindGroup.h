#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanDevice.h"
#include "Platform\Vulkan\VulkanRenderer.h"

#include "Platform\Vulkan\VulkanCommon.h"

namespace HBL2
{
	struct VulkanBindGroupHot
	{
		VkDescriptorSet DescriptorSet = VK_NULL_HANDLE; // Hot
	};

	struct VulkanBindGroupCold
	{
		const char* DebugName = "";
		Handle<BindGroupLayout> BindGroupLayout;
		std::vector<Handle<Texture>> Textures;
		std::vector<BindGroupDescriptor::BufferEntry> Buffers;

		void Destroy();
	};

	// Helper struct for centralised operations on hot and cold data.
	// NOTE: Use with SplitPool::Get to retrieve the Hot and Cold data from the pool.
	struct VulkanBindGroup
	{
		VulkanBindGroup() = default;

		bool IsValid() const;

		void Initialize(const BindGroupDescriptor&& desc);
		void Update();
		void Destroy();

		VulkanBindGroupHot* Hot = nullptr;
		VulkanBindGroupCold* Cold = nullptr;
	};
}