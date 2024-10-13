#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanDevice.h"
#include "Platform\Vulkan\VulkanRenderer.h"

#include "Platform\Vulkan\VulkanCommon.h"

namespace HBL2
{
	struct VulkanBuffer
	{
		VulkanBuffer() = default;
		VulkanBuffer(const BufferDescriptor&& desc)
		{
			/*
			VulkanDevice* device = (VulkanDevice*)Device::Instance;
			VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

			DebugName = desc.debugName;

			VkBufferCreateInfo bufferCreateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = desc.byteSize,
				.usage = BufferUsageToVkBufferUsageFlags(desc.usage),
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 0,
				.pQueueFamilyIndices = nullptr,
			};

			VmaAllocationCreateInfo vmaAllocInfo =
			{
				.usage = MemoryUsageToVmaMemoryUsage(desc.memoryUsage),
			};

			VK_VALIDATE(vmaCreateBuffer(renderer->GetAllocator(), &bufferCreateInfo, &vmaAllocInfo, &Buffer, &Allocation, nullptr), "vmaCreateImage");
			*/
		}

		VkBufferUsageFlags BufferUsageToVkBufferUsageFlags(BufferUsage bufferUsage)
		{
			switch (bufferUsage)
			{
			case HBL2::BufferUsage::MAP_READ:
				break;
			case HBL2::BufferUsage::MAP_WRITE:
				break;
			case HBL2::BufferUsage::COPY_SRC:
				break;
			case HBL2::BufferUsage::COPY_DST:
				break;
			case HBL2::BufferUsage::INDEX:
				return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			case HBL2::BufferUsage::VERTEX:
				return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			case HBL2::BufferUsage::UNIFORM:
				return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			case HBL2::BufferUsage::STORAGE:
				return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			case HBL2::BufferUsage::INDIRECT:
				return VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
			case HBL2::BufferUsage::QUERY_RESOLVE:
				break;
			default:
				break;
			}

			return 0;
		}

		VmaMemoryUsage MemoryUsageToVmaMemoryUsage(MemoryUsage memoryUsage)
		{
			switch (memoryUsage)
			{
			case HBL2::MemoryUsage::CPU_CPU:
				return VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_ONLY;
			case HBL2::MemoryUsage::GPU_CPU:
				return VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_TO_CPU;
			case HBL2::MemoryUsage::CPU_GPU:
				return VmaMemoryUsage::VMA_MEMORY_USAGE_CPU_TO_GPU;
			}
		}

		const char* DebugName = "";
		VkBuffer Buffer;
		VmaAllocation Allocation;
	};
}