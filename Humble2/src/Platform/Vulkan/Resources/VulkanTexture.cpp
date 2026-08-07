#include "VulkanTexture.h"

namespace HBL2
{
    static BlockFormatInfo GetBlockFormatInfo(Format format)
    {
        switch (format)
        {
        case Format::D32_FLOAT:
        case Format::RGBA8_RGB:
        case Format::RGBA8_UNORM:
        case Format::BGRA8_UNORM:
        case Format::RG16_FLOAT:
            return { 1, 1, 4 };
        case Format::RGBA32_FLOAT:
            return { 1, 1, 16 };
        case Format::RGBA16_FLOAT:
        case Format::RGB32_FLOAT:
            return { 1, 1, 8 };
        case Format::R10G10B10A2_UNORM:
            return { 1, 1, 8 }; // TODO: same open question as the Vulkan version.
        case Format::ASTC_4x4_SRGB:
        case Format::ASTC_4x4_UNORM:
            return { 4, 4, 16 };
        case Format::ASTC_6x6_SRGB:
        case Format::ASTC_6x6_UNORM:
            return { 6, 6, 16 };
        case Format::ASTC_8x8_SRGB:
        case Format::ASTC_8x8_UNORM:
            return { 8, 8, 16 };
        case Format::ASTC_10x10_SRGB:
        case Format::ASTC_10x10_UNORM:
            return { 10, 10, 16 };
        case Format::ASTC_12x12_SRGB:
        case Format::ASTC_12x12_UNORM:
            return { 12, 12, 16 };

        // case Format::BC1_RGBA_UNORM: return { 4, 4, 8 };  // BC1/BC4
        // case Format::BC3_RGBA_UNORM: return { 4, 4, 16 }; // BC2/3/5/6H/7

        default:
            return { 1, 1, 4 };
        }
    }

    static inline uint32_t DivRoundUp(uint32_t value, uint32_t divisor)
    {
        return (value + divisor - 1) / divisor;
    }

    static inline size_t RowPitch(uint32_t width, const BlockFormatInfo& info)
    {
        return (size_t)DivRoundUp(width, info.blockWidth) * info.bytesPerBlock;
    }

    static inline size_t ImageSize(uint32_t width, uint32_t height, const BlockFormatInfo& info)
    {
        return RowPitch(width, info) * DivRoundUp(height, info.blockHeight);
    }

	VulkanTexture::VulkanTexture(const TextureDescriptor&& desc)
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

		DebugName = desc.debugName;

		ImageLayout = VkUtils::TextureLayoutToVkImageLayout(desc.initialLayout);
		ImageType = desc.type;
		LayerCount = desc.layerCount;
		Extent = { desc.dimensions.x, desc.dimensions.y, desc.dimensions.z };
		Aspect = VkUtils::TextureAspectToVkImageAspectFlags(desc.aspect);
        
        m_BlockInfo = GetBlockFormatInfo(desc.format);

		VkImageUsageFlags usage = VkUtils::TextureUsageFlagToVkImageUsageFlags(desc.usage);

		// Allocate Image
		VkImageCreateInfo imageCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = (VkImageCreateFlags)(desc.type == TextureType::CUBE || desc.type == TextureType::D2_ARRAY ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0),
			.imageType = VkUtils::TextureTypeToVkImageType(desc.type),
			.format = VkUtils::FormatToVkFormat(desc.format),
			.extent = Extent,
			.mipLevels = 1,
			.arrayLayers = (uint32_t)(desc.type == TextureType::CUBE ? 6 : LayerCount),
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = usage,
			//.initialLayout = ImageLayout,
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

            VkDeviceSize imageSize = (VkDeviceSize)ImageSize(Extent.width, Extent.height, m_BlockInfo);

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

            VkDeviceSize faceSize = (VkDeviceSize)ImageSize(Extent.width, Extent.height, m_BlockInfo);
			VkDeviceSize imageSize = faceSize * (ImageType == TextureType::CUBE ? 6 : LayerCount);

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
				.layerCount = (uint32_t)(desc.type == TextureType::CUBE ? 6 : LayerCount),
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

			if (desc.sampler.wrap == Wrap::CLAMP_TO_BORDER)
			{
				samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			}

			VK_VALIDATE(vkCreateSampler(device->Get(), &samplerInfo, nullptr, &Sampler), "vkCreateSampler");
		}
	}

	VulkanTexture::VulkanTexture(const VulkanTexture&& other) noexcept
	{
		Image = other.Image;
		ImageView = other.ImageView;
		Extent = other.Extent;
		Aspect = other.Aspect;
		LayerCount = other.LayerCount;
	}

	void VulkanTexture::Update(const Span<const std::byte>& bytes)
	{
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

		CreateStagingBuffer(renderer, &stagingBuffer, &stagingBufferAllocation);

        VkDeviceSize faceSize = (VkDeviceSize)ImageSize(Extent.width, Extent.height, m_BlockInfo);
		VkDeviceSize imageSize = faceSize * (ImageType == TextureType::CUBE ? 6 : LayerCount);

		// Transfer initiaData to staging buffer
		void* mappedData;
		vmaMapMemory(renderer->GetAllocator(), stagingBufferAllocation, &mappedData);
		memcpy(mappedData, bytes.Data(), (size_t)imageSize);
		vmaUnmapMemory(renderer->GetAllocator(), stagingBufferAllocation);

		CopyBufferToTexture(renderer, stagingBuffer);

		vmaDestroyBuffer(renderer->GetAllocator(), stagingBuffer, stagingBufferAllocation);
	}

	void VulkanTexture::ChangeTextureView(const TextureViewDescriptor&& desc)
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;

		// Destroy old ImageView
		VkImageView oldImageView = ImageView;
		auto& deletionQueue = ResourceManager::Instance->GetDeletionQueue();
		deletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=]()
		{
			vkDestroyImageView(device->Get(), oldImageView, nullptr);
		});

		// Allocate ImageView
		VkImageViewCreateInfo imageViewCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.pNext = nullptr,
			.image = Image,
			.viewType = VkUtils::TextureTypeToVkVkImageViewType(desc.type),
			.format = VkUtils::FormatToVkFormat(desc.format),
			.subresourceRange =
			{
				.aspectMask = VkUtils::TextureAspectToVkImageAspectFlags(desc.aspect),
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = desc.layerCount,
			},
		};

		VK_VALIDATE(vkCreateImageView(device->Get(), &imageViewCreateInfo, nullptr, &ImageView), "vkCreateImageView");
	}

	void VulkanTexture::TransitionLayout(VulkanCommandBuffer* commandBuffer, ResourceState currentState, ResourceState newState)
	{
        commandBuffer->TextureBarrier(this, currentState, newState);
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
        VkDeviceSize faceSize = (VkDeviceSize)ImageSize(Extent.width, Extent.height, m_BlockInfo);
		VkDeviceSize imageSize = faceSize * (ImageType == TextureType::CUBE ? 6 : LayerCount);

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
		renderer->ImmediateSubmit([=, this](VkCommandBuffer cmd)
		{
			uint32_t faceCount = (ImageType == TextureType::CUBE ? 6 : LayerCount);

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

            VkDeviceSize faceSize = (VkDeviceSize)ImageSize(Extent.width, Extent.height, m_BlockInfo);
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
