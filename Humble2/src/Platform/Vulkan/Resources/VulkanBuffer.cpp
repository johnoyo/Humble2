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
#if DEBUG
			.flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT,
#endif
			.usage = Cold->MemoryUsage,
#if DEBUG
			.pUserData = (void*)Cold->DebugName,
#endif
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

			renderer->ImmediateSubmit([=, this](VkCommandBuffer cmd)
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
			renderer->ImmediateSubmit([=, this](VkCommandBuffer cmd)
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

	bool VulkanBuffer::ReadBack(void* outData, uint64_t size, uint64_t offset) const
	{
		if (size == 0)
		{
			size = Hot->ByteSize;
		}

		if (!IsValid() || outData == nullptr || size == 0)
		{
			return false;
		}

		if (offset + size > Hot->ByteSize)
		{
			return false;
		}

		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;
		VmaAllocator allocator = renderer->GetAllocator();

		// Fast path: memory is already host visible (UMA / ReBAR / CPU_TO_GPU buffers).
		VmaAllocationInfo allocInfo{};
		vmaGetAllocationInfo(allocator, Hot->Allocation, &allocInfo);

		VkMemoryPropertyFlags memFlags = 0;
		vmaGetMemoryTypeProperties(allocator, allocInfo.memoryType, &memFlags);

		if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		{
			void* mappedData = nullptr;
			if (vmaMapMemory(allocator, Hot->Allocation, &mappedData) == VK_SUCCESS)
			{
				// No-op on coherent memory, required on non-coherent.
				vmaInvalidateAllocation(allocator, Hot->Allocation, offset, size);
				memcpy(outData, (const uint8_t*)mappedData + offset, size);
				vmaUnmapMemory(allocator, Hot->Allocation);
				return true;
			}
		}

		// Slow path: GPU -> staging -> CPU.
		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

		VkBufferCreateInfo stagingBufferCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
		};

		VmaAllocationCreateInfo vmaStagingAllocCreateInfo =
		{
			.usage = VMA_MEMORY_USAGE_GPU_TO_CPU, // host visible + cached, best for CPU reads.
		};

		VK_VALIDATE(vmaCreateBuffer(allocator, &stagingBufferCreateInfo, &vmaStagingAllocCreateInfo, &stagingBuffer, &stagingBufferAllocation, nullptr), "vmaCreateBuffer");

		renderer->ImmediateSubmit([=, this](VkCommandBuffer cmd)
		{
			// Make sure any prior GPU writes to the source are visible to the transfer read.
			VkBufferMemoryBarrier barrier =
			{
				.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.buffer = Hot->Buffer,
				.offset = offset,
				.size = size,
			};

			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0, 0, nullptr, 1, &barrier, 0, nullptr);

			VkBufferCopy copy =
			{
				.srcOffset = offset,
				.dstOffset = 0,
				.size = size,
			};

			vkCmdCopyBuffer(cmd, Hot->Buffer, stagingBuffer, 1, &copy);
		});

		// ImmediateSubmit waits on its fence, so the copy is complete and host-visible here.
		void* mappedData = nullptr;
		bool success = false;

		if (vmaMapMemory(allocator, stagingBufferAllocation, &mappedData) == VK_SUCCESS)
		{
			vmaInvalidateAllocation(allocator, stagingBufferAllocation, 0, size);
			memcpy(outData, mappedData, size);
			vmaUnmapMemory(allocator, stagingBufferAllocation);
			success = true;
		}

		vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);

		return success;
	}
	bool VulkanBuffer::ReadBack(ByteSpan& outBufferBytes, uint64_t offset) const
	{
		if (!ReadBack(outBufferBytes.data(), outBufferBytes.size(), offset))
		{
			return false;
		}

		return true;
	}
}
