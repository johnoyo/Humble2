#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanDevice.h"
#include "Platform\Vulkan\VulkanRenderer.h"

#include "Platform\Vulkan\VulkanCommon.h"

namespace HBL2
{
	struct VulkanBufferHot
	{
		VkBuffer Buffer = VK_NULL_HANDLE;
		VmaAllocation Allocation = VK_NULL_HANDLE;
		void* Data = nullptr;
		uint32_t ByteSize = 0;

		void Destroy();
	};

	struct VulkanBufferCold
	{
		void ReAllocate(uint32_t currentOffset);

		const char* DebugName = "";
		uint32_t ByteOffset = 0;
		VkBufferUsageFlags BufferUsageFlags = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
		VmaMemoryUsage MemoryUsage = VMA_MEMORY_USAGE_MAX_ENUM;
	};

	struct VulkanBuffer
	{
		VulkanBuffer() = default;

		bool IsValid() const;

		void Initialize(const BufferDescriptor&& desc);
		void ReAllocate(uint32_t currentOffset);
		void Destroy();

		VulkanBufferHot* Hot;
		VulkanBufferCold* Cold;
	};
}