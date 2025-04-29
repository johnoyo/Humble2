#include "VulkanTexture.h"

namespace HBL2
{
	VulkanTexture::VulkanTexture(const TextureDescriptor&& desc)
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

		DebugName = desc.debugName;

		ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // NOTE: Is this a correct default value? should it be Undefined?
		ImageType = desc.type;
		Extent = { desc.dimensions.x, desc.dimensions.y, desc.dimensions.z };
		Aspect = VkUtils::TextureAspectToVkImageAspectFlags(desc.aspect);

		VkImageUsageFlags usage = VkUtils::TextureUsageFlagToVkImageUsageFlags(desc.usage);

		// Allocate Image
		VkImageCreateInfo imageCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = (VkImageCreateFlags)(desc.type == TextureType::CUBE ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0),
			.imageType = VkUtils::TextureTypeToVkImageType(desc.type),
			.format = VkUtils::FormatToVkFormat(desc.format),
			.extent = Extent,
			.mipLevels = 1,
			.arrayLayers = (uint32_t)(desc.type == TextureType::CUBE ? 6 : 1),
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = usage,
		};

		VmaAllocationCreateInfo allocationCreateInfo =
		{
			.usage = VMA_MEMORY_USAGE_GPU_ONLY,
			.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
		};

		VK_VALIDATE(vmaCreateImage(renderer->GetAllocator(), &imageCreateInfo, &allocationCreateInfo, &Image, &Allocation, nullptr), "vmaCreateImage");

		if (desc.initialData == nullptr && Extent.width == 1 && Extent.height == 1)
		{
			VkBuffer stagingBuffer = VK_NULL_HANDLE;
			VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

			CreateStagingBuffer(renderer, &stagingBuffer, &stagingBufferAllocation);

			VkDeviceSize imageSize = Extent.width * Extent.height * 4;

			uint32_t whiteTexture = 0xffffffff;

			// Transfer initiaData to staging buffer
			void* mappedData;
			vmaMapMemory(renderer->GetAllocator(), stagingBufferAllocation, &mappedData);
			memcpy(mappedData, &whiteTexture, (size_t)imageSize);
			vmaUnmapMemory(renderer->GetAllocator(), stagingBufferAllocation);

			CopyBufferToTexture(renderer, stagingBuffer);

			vmaDestroyBuffer(renderer->GetAllocator(), stagingBuffer, stagingBufferAllocation);
		}
		else if (desc.initialData != nullptr)
		{
			VkBuffer stagingBuffer = VK_NULL_HANDLE;
			VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

			CreateStagingBuffer(renderer, &stagingBuffer, &stagingBufferAllocation);

			VkDeviceSize imageSize = Extent.width * Extent.height * 4;

			// Transfer initiaData to staging buffer
			void* mappedData;
			vmaMapMemory(renderer->GetAllocator(), stagingBufferAllocation, &mappedData);
			memcpy(mappedData, desc.initialData, (size_t)imageSize);
			vmaUnmapMemory(renderer->GetAllocator(), stagingBufferAllocation);

			stbi_image_free(desc.initialData);

			CopyBufferToTexture(renderer, stagingBuffer);

			vmaDestroyBuffer(renderer->GetAllocator(), stagingBuffer, stagingBufferAllocation);
		}

		// Allocate ImageView
		VkImageViewCreateInfo imageViewCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.image = Image,
			.viewType = VkUtils::TextureTypeToVkVkImageViewType(desc.type),
			.format = VkUtils::FormatToVkFormat(desc.internalFormat),
			.subresourceRange =
			{
				.aspectMask = VkUtils::TextureAspectToVkImageAspectFlags(desc.aspect),
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = (uint32_t)(desc.type == TextureType::CUBE ? 6 : 1),
			},
		};

		VK_VALIDATE(vkCreateImageView(device->Get(), &imageViewCreateInfo, nullptr, &ImageView), "vkCreateImageView");

		if (desc.createSampler)
		{
			// Create sampler
			VkSamplerCreateInfo samplerInfo =
			{
				.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
				.pNext = nullptr,
				.magFilter = VkUtils::FilterToVkFilter(desc.sampler.filter),
				.minFilter = VkUtils::FilterToVkFilter(desc.sampler.filter),
				.addressModeU = VkUtils::WrapToVkSamplerAddressMode(desc.sampler.wrap),
				.addressModeV = VkUtils::WrapToVkSamplerAddressMode(desc.sampler.wrap),
				.addressModeW = VkUtils::WrapToVkSamplerAddressMode(desc.sampler.wrap),
				.compareOp = desc.sampler.compareEnable ? VkUtils::CompareToVkCompareOp(desc.sampler.compare) : VK_COMPARE_OP_NEVER,
			};

			VK_VALIDATE(vkCreateSampler(device->Get(), &samplerInfo, nullptr, &Sampler), "vkCreateSampler");
		}
	}

	void VulkanTexture::Update(const Span<const std::byte>& bytes)
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

		CreateStagingBuffer(renderer, &stagingBuffer, &stagingBufferAllocation);

		VkDeviceSize faceSize = Extent.width * Extent.height * 4;
		VkDeviceSize imageSize = faceSize * (ImageType == TextureType::CUBE ? 6 : 1);

		// Transfer initiaData to staging buffer
		void* mappedData;
		vmaMapMemory(renderer->GetAllocator(), stagingBufferAllocation, &mappedData);
		memcpy(mappedData, bytes.Data(), (size_t)imageSize);
		vmaUnmapMemory(renderer->GetAllocator(), stagingBufferAllocation);

		CopyBufferToTexture(renderer, stagingBuffer);

		vmaDestroyBuffer(renderer->GetAllocator(), stagingBuffer, stagingBufferAllocation);
	}

	void VulkanTexture::TrasitionLayout(VulkanCommandBuffer* commandBuffer, TextureLayout currentLayout, TextureLayout newLayout, VulkanBindGroup* bindGroup)
	{
		// NOTE: Vulkan validation layers do not automatically track the layout state across command buffers unless you properly synchronize them.
		//		 That's way we do not use here the helper function ImmediateSubmit of the renderer class here.

		VkCommandBuffer cmd = commandBuffer->CommandBuffer;

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VkUtils::TextureLayoutToVkImageLayout(currentLayout);
		barrier.newLayout = VkUtils::TextureLayoutToVkImageLayout(newLayout);
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = Image;
		barrier.subresourceRange.aspectMask = Aspect;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = VkUtils::CurrentTextureLayoutToVkAccessFlags(currentLayout);
		barrier.dstAccessMask = VkUtils::CurrentTextureLayoutToVkAccessFlags(newLayout);

		vkCmdPipelineBarrier(
			cmd,
			VkUtils::CurrentTextureLayoutToVkPipelineStageFlags(currentLayout),
			VkUtils::NewTextureLayoutToVkPipelineStageFlags(newLayout),
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		// Wait for device to idle in case the descriptors are still in use.
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		vkDeviceWaitIdle(device->Get());

		// Update bind group with new image layout.
		ImageLayout = VkUtils::TextureLayoutToVkImageLayout(newLayout);

		if (bindGroup != nullptr)
		{
			bindGroup->Update();
		}
	}

	void VulkanTexture::Destroy()
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

		if (Sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(device->Get(), Sampler, nullptr);
		}

		vkDestroyImageView(device->Get(), ImageView, nullptr);
		vmaDestroyImage(renderer->GetAllocator(), Image, Allocation);
	}

	void VulkanTexture::CreateStagingBuffer(VulkanRenderer* renderer, VkBuffer* stagingBuffer, VmaAllocation* stagingBufferAllocation)
	{
		// Allocate staging buffer
		VkDeviceSize faceSize = Extent.width * Extent.height * 4;
		VkDeviceSize imageSize = faceSize * (ImageType == TextureType::CUBE ? 6 : 1);

		VkBufferCreateInfo stagingBufferCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = imageSize,
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.queueFamilyIndexCount = 0,
			.pQueueFamilyIndices = nullptr,
		};

		VmaAllocationCreateInfo vmaStagingAllocCreateInfo =
		{
			.usage = VMA_MEMORY_USAGE_CPU_ONLY,
		};

		VK_VALIDATE(vmaCreateBuffer(renderer->GetAllocator(), &stagingBufferCreateInfo, &vmaStagingAllocCreateInfo, stagingBuffer, stagingBufferAllocation, nullptr), "vmaCreateBuffer");
	}

	void VulkanTexture::CopyBufferToTexture(VulkanRenderer* renderer, VkBuffer stagingBuffer)
	{
		// Copy the data of the staging buffer to the GPU memory of Image
		renderer->ImmediateSubmit([=](VkCommandBuffer cmd)
		{
			uint32_t faceCount = (ImageType == TextureType::CUBE ? 6 : 1);

			VkImageSubresourceRange range =
			{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = faceCount,
			};

			VkImageMemoryBarrier imageBarrierToTransfer =
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.image = Image,
				.subresourceRange = range,
			};

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrierToTransfer);

			VkDeviceSize faceSize = Extent.width * Extent.height * 4;
			StaticArray<VkBufferImageCopy, 6> copyRegions{};

			for (uint32_t face = 0; face < faceCount; ++face)
			{
				copyRegions[face] =
				{
					.bufferOffset = faceSize * face,
					.bufferRowLength = 0,
					.bufferImageHeight = 0,
					.imageSubresource =
					{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.mipLevel = 0,
						.baseArrayLayer = face,
						.layerCount = 1,
					},
					.imageExtent = Extent,
				};
			}

			vkCmdCopyBufferToImage(cmd, stagingBuffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, faceCount, copyRegions.Data());

			VkImageMemoryBarrier imageBarrierToReadable =
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.image = Image,
				.subresourceRange = range,
			};

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrierToReadable);
		});
	}
}