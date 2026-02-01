#include "VulkanBuffer.h"

namespace HBL2
{
	void VulkanBufferHot::Destroy()
	{
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;
		vmaDestroyBuffer(renderer->GetAllocator(), Buffer, Allocation);
	}

	bool VulkanBuffer::IsValid() const
	{
		return Cold != nullptr && Hot != nullptr;
	}

	void VulkanBuffer::Initialize(const BufferDescriptor&& desc)
	{
		if (!IsValid())
		{
			return;
		}

		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

		Cold->DebugName = desc.debugName;
		Hot->ByteSize = desc.byteSize;
		Cold->BufferUsageFlags = VkUtils::BufferUsageToVkBufferUsageFlags(desc.usage);
		Cold->MemoryUsage = VkUtils::MemoryUsageToVmaMemoryUsage(desc.memoryUsage);

		VkBufferCreateInfo bufferCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = Hot->ByteSize,
			.usage = Cold->BufferUsageFlags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, // NOTE: Investigate if always needs VK_IMAGE_USAGE_TRANSFER_DST_BIT
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
		};

		VmaAllocationCreateInfo vmaAllocCreateInfo =
		{
			.usage = Cold->MemoryUsage,
		};

		VK_VALIDATE(vmaCreateBuffer(renderer->GetAllocator(), &bufferCreateInfo, &vmaAllocCreateInfo, &Hot->Buffer, &Hot->Allocation, nullptr), "vmaCreateBuffer");

		if (desc.initialData != nullptr)
		{
			Hot->Data = desc.initialData;

			VkBuffer stagingBuffer = VK_NULL_HANDLE;
			VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

			VkBufferCreateInfo stagingBufferCreateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				.size = Hot->ByteSize,
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
			memcpy(mappedData, Hot->Data, Hot->ByteSize);
			vmaUnmapMemory(renderer->GetAllocator(), stagingBufferAllocation);

			renderer->ImmediateSubmit([=](VkCommandBuffer cmd)
			{
				VkBufferCopy copy =
				{
					.srcOffset = 0,
					.dstOffset = 0,
					.size = Hot->ByteSize,
				};

				vkCmdCopyBuffer(cmd, stagingBuffer, Hot->Buffer, 1, &copy);
			});

			vmaDestroyBuffer(renderer->GetAllocator(), stagingBuffer, stagingBufferAllocation);
		}
	}

	void VulkanBuffer::ReAllocate(uint32_t currentOffset)
	{
		if (!IsValid())
		{
			return;
		}

		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

		// Store old buffer and allocation
		VkBuffer oldBuffer = Hot->Buffer;
		VmaAllocation oldAllocation = Hot->Allocation;

		// Create new buffer info based on the original descriptor but a new size
		VkBufferCreateInfo bufferCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = Hot->ByteSize * 2,
			.usage = Cold->BufferUsageFlags | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
		};

		// Memory allocation info
		VmaAllocationCreateInfo allocCreateInfo =
		{
			.usage = Cold->MemoryUsage,
		};

		// Create the new buffer and allocation
		VK_VALIDATE(vmaCreateBuffer(renderer->GetAllocator(), &bufferCreateInfo, &allocCreateInfo, &Hot->Buffer, &Hot->Allocation, nullptr), "Failed to reallocate buffer");

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

				vkCmdCopyBuffer(cmd, oldBuffer, Hot->Buffer, 1, &copyRegion);
			});
		}

		Hot->ByteSize = Hot->ByteSize * 2;

		// Destroy the old buffer
		vmaDestroyBuffer(renderer->GetAllocator(), oldBuffer, oldAllocation);
	}

	void VulkanBuffer::Destroy()
	{
		if (!IsValid())
		{
			return;
		}

		Hot->Destroy();
	}
}