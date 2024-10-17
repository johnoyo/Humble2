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
			VulkanDevice* device = (VulkanDevice*)Device::Instance;
			VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

			DebugName = desc.debugName;
			ByteSize = desc.byteSize;

			VkBufferCreateInfo bufferCreateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = ByteSize,
				.usage = BufferUsageToVkBufferUsageFlags(desc.usage),
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 0,
				.pQueueFamilyIndices = nullptr,
			};

			VmaAllocationCreateInfo vmaAllocCreateInfo =
			{
				.usage = MemoryUsageToVmaMemoryUsage(desc.memoryUsage),
			};

			VK_VALIDATE(vmaCreateBuffer(renderer->GetAllocator(), &bufferCreateInfo, &vmaAllocCreateInfo, &Buffer, &Allocation, nullptr), "vmaCreateBuffer");

			if (desc.initialData != nullptr)
			{
				Data = desc.initialData;

				VkBuffer stagingBuffer = VK_NULL_HANDLE;
				VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

				VkBufferCreateInfo stagingBufferCreateInfo =
				{
					.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
					.size = ByteSize,
					.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
					.queueFamilyIndexCount = 0,
					.pQueueFamilyIndices = nullptr,
				};

				VmaAllocationCreateInfo vmaStagingAllocCreateInfo =
				{
					.usage = VMA_MEMORY_USAGE_CPU_ONLY,
				};

				VK_VALIDATE(vmaCreateBuffer(renderer->GetAllocator(), &stagingBufferCreateInfo, &vmaStagingAllocCreateInfo, &stagingBuffer, &stagingBufferAllocation, nullptr), "vmaCreateBuffer");

				void* mappedData;
				vmaMapMemory(renderer->GetAllocator(), stagingBufferAllocation, &mappedData);
				memcpy(mappedData, Data, ByteSize);
				vmaUnmapMemory(renderer->GetAllocator(), stagingBufferAllocation);

				renderer->ImmediateSubmit([=](VkCommandBuffer cmd)
				{
					VkBufferCopy copy =
					{
						.srcOffset = 0,
						.dstOffset = 0,
						.size = ByteSize,
					};
					
					vkCmdCopyBuffer(cmd, stagingBuffer, Buffer, 1, &copy);
				});

				vmaDestroyBuffer(renderer->GetAllocator(), stagingBuffer, stagingBufferAllocation);
			}
		}

		const char* DebugName = "";
		VkBuffer Buffer = VK_NULL_HANDLE;
		VmaAllocation Allocation = VK_NULL_HANDLE;
		VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
		uint32_t ByteOffset = 0;
		uint32_t ByteSize = 0;
		void* Data = nullptr;

	private:
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

			return VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
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

			return VMA_MEMORY_USAGE_MAX_ENUM;
		}
	};
}