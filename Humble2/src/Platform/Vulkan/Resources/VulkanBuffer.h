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
				.usage = VkUtils::BufferUsageToVkBufferUsageFlags(desc.usage) | VK_BUFFER_USAGE_TRANSFER_DST_BIT, // NOTE: Investigate if always needs VK_IMAGE_USAGE_TRANSFER_DST_BIT
				.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
				.queueFamilyIndexCount = 0,
				.pQueueFamilyIndices = nullptr,
			};

			VmaAllocationCreateInfo vmaAllocCreateInfo =
			{
				.usage = VkUtils::MemoryUsageToVmaMemoryUsage(desc.memoryUsage),
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
			if (DescriptorSet == VK_NULL_HANDLE)
			{
				HBL2_CORE_WARN("Buffer with name: {0} is set for static usage, re-allocating will have no effect", DebugName);
				return;
			}
		}

		void Destroy()
		{
			VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

			vmaDestroyBuffer(renderer->GetAllocator(), Buffer, Allocation);
		}

		const char* DebugName = "";
		VkBuffer Buffer = VK_NULL_HANDLE;
		VmaAllocation Allocation = VK_NULL_HANDLE;
		VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
		uint32_t ByteOffset = 0;
		uint32_t ByteSize = 0;
		void* Data = nullptr;
	};
}