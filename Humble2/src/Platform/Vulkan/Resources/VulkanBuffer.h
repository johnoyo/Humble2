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
			BufferUsageFlags = VkUtils::BufferUsageToVkBufferUsageFlags(desc.usage);
			MemoryUsage = VkUtils::MemoryUsageToVmaMemoryUsage(desc.memoryUsage);

			VkBufferCreateInfo bufferCreateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = ByteSize,
				.usage = BufferUsageFlags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, // NOTE: Investigate if always needs VK_IMAGE_USAGE_TRANSFER_DST_BIT
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 0,
				.pQueueFamilyIndices = nullptr,
			};

			VmaAllocationCreateInfo vmaAllocCreateInfo =
			{
				.usage = MemoryUsage,
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

		void ReAllocate(uint32_t currentOffset)
		{
			VulkanDevice* device = (VulkanDevice*)Device::Instance;
			VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

			// Store old buffer and allocation
			VkBuffer oldBuffer = Buffer;
			VmaAllocation oldAllocation = Allocation;

			// Create new buffer info based on the original descriptor but a new size
			VkBufferCreateInfo bufferCreateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = ByteSize * 2,
				.usage = BufferUsageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 0,
				.pQueueFamilyIndices = nullptr,
			};

			// Memory allocation info
			VmaAllocationCreateInfo allocCreateInfo =
			{
				.usage = MemoryUsage,
			};

			// Create the new buffer and allocation
			VK_VALIDATE(vmaCreateBuffer(renderer->GetAllocator(), &bufferCreateInfo, &allocCreateInfo, &Buffer, &Allocation, nullptr), "Failed to reallocate buffer");

			// Check if there is existing data to copy from the old buffer
			if (oldBuffer != VK_NULL_HANDLE && currentOffset > 0)
			{
				renderer->ImmediateSubmit([=](VkCommandBuffer cmd)
				{
					VkBufferCopy copyRegion =
					{
						.srcOffset = 0,
						.dstOffset = 0,
						.size = currentOffset,  // Copy up to currentOffset bytes
					};

					vkCmdCopyBuffer(cmd, oldBuffer, Buffer, 1, &copyRegion);
				});
			}

			ByteSize = ByteSize * 2;

			// Destroy the old buffer
			vmaDestroyBuffer(renderer->GetAllocator(), oldBuffer, oldAllocation);
		}

		void Destroy()
		{
			VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;
			vmaDestroyBuffer(renderer->GetAllocator(), Buffer, Allocation);
		}

		const char* DebugName = "";
		VkBuffer Buffer = VK_NULL_HANDLE;
		VmaAllocation Allocation = VK_NULL_HANDLE;
		uint32_t ByteOffset = 0;
		uint32_t ByteSize = 0;
		VkBufferUsageFlags BufferUsageFlags = VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM;
		VmaMemoryUsage MemoryUsage = VMA_MEMORY_USAGE_MAX_ENUM;
		void* Data = nullptr;
	};
}