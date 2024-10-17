#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanDevice.h"
#include "Platform\Vulkan\VulkanRenderer.h"

#include "Platform\Vulkan\VulkanCommon.h"

namespace HBL2
{
	struct VulkanTexture
	{
		VulkanTexture() = default;
		VulkanTexture(const TextureDescriptor&& desc)
		{
			VulkanDevice* device = (VulkanDevice*)Device::Instance;
			VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

			DebugName = desc.debugName;

			Extent = { desc.dimensions.x, desc.dimensions.y, desc.dimensions.z };

			// Allocate Image
			VkImageCreateInfo imageCreateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.pNext = nullptr,
				.imageType = TextureTypeToVkImageType(desc.type),
				.format = FormatToVkFormat(desc.format),
				.extent = Extent,
				.mipLevels = 1,
				.arrayLayers = 1,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.tiling = VK_IMAGE_TILING_OPTIMAL,
				.usage = TextureUsageToVkImageLayout(desc.usage) | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			};
			
			VmaAllocationCreateInfo allocationCreateInfo =
			{
				.usage = VMA_MEMORY_USAGE_GPU_ONLY,
				.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
			};
			
			VK_VALIDATE(vmaCreateImage(renderer->GetAllocator(), &imageCreateInfo, &allocationCreateInfo, &Image, &Allocation, nullptr), "vmaCreateImage");

			// Transfer initiaData to staging buffer
			if (desc.initialData == nullptr && Extent.width == 1 && Extent.height == 1)
			{
				VkBuffer stagingBuffer = VK_NULL_HANDLE;
				VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

				CreateStagingBuffer(renderer, &stagingBuffer, &stagingBufferAllocation);

				VkDeviceSize imageSize = Extent.width * Extent.height * 4;

				uint32_t whiteTexture = 0xffffffff;

				void* mappedData;
				vmaMapMemory(renderer->GetAllocator(), stagingBufferAllocation, &mappedData);
				memcpy(mappedData, &whiteTexture, (size_t)imageSize);
				vmaUnmapMemory(renderer->GetAllocator(), stagingBufferAllocation);

				CopyBufferTotexture(renderer, stagingBuffer);
			}
			else if (desc.initialData != nullptr)
			{
				VkBuffer stagingBuffer = VK_NULL_HANDLE;
				VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

				CreateStagingBuffer(renderer, &stagingBuffer, &stagingBufferAllocation);

				VkDeviceSize imageSize = Extent.width * Extent.height * 4;

				void* mappedData;
				vmaMapMemory(renderer->GetAllocator(), stagingBufferAllocation, &mappedData);
				memcpy(mappedData, desc.initialData, (size_t)imageSize);
				vmaUnmapMemory(renderer->GetAllocator(), stagingBufferAllocation);

				stbi_image_free(desc.initialData);

				CopyBufferTotexture(renderer, stagingBuffer);
			}

			// Allocate ImageView
			VkImageViewCreateInfo imageViewCreateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.pNext = nullptr,
				.image = Image,
				.viewType = TextureTypeToVkVkImageViewType(desc.type),
				.format = FormatToVkFormat(desc.internalFormat),
				.subresourceRange = 
				{
					.aspectMask = TextureAspectToVkImageAspectFlags(desc.aspect),
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
				}
			};

			VK_VALIDATE(vkCreateImageView(device->Get(), &imageViewCreateInfo, nullptr, &ImageView), "vkCreateImageView");
		}

		const char* DebugName = "";
		VkImage Image = VK_NULL_HANDLE;
		VkImageView ImageView = VK_NULL_HANDLE;
		VmaAllocation Allocation = VK_NULL_HANDLE;
		VkExtent3D Extent{};

	private:
		void CreateStagingBuffer(VulkanRenderer* renderer, VkBuffer* stagingBuffer, VmaAllocation* stagingBufferAllocation)
		{
			// Allocate staging buffer
			VkDeviceSize imageSize = Extent.width * Extent.height * 4;

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

		void CopyBufferTotexture(VulkanRenderer* renderer, VkBuffer stagingBuffer)
		{
			// Copy the data of the staging buffer to the GPU memory of Image
			renderer->ImmediateSubmit([&](VkCommandBuffer cmd)
			{
				VkImageSubresourceRange range =
				{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1,
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

				VkBufferImageCopy copyRegion =
				{
					.bufferOffset = 0,
					.bufferRowLength = 0,
					.bufferImageHeight = 0,
					.imageSubresource =
					{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.mipLevel = 0,
						.baseArrayLayer = 0,
						.layerCount = 1,
					},
					.imageExtent = Extent,
				};

				vkCmdCopyBufferToImage(cmd, stagingBuffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

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

		VkImageType TextureTypeToVkImageType(TextureType textureType)
		{
			switch (textureType)
			{
			case HBL2::TextureType::D1:
				return VK_IMAGE_TYPE_1D;
			case HBL2::TextureType::D2:
				return VK_IMAGE_TYPE_2D;
			case HBL2::TextureType::D3:
				return VK_IMAGE_TYPE_3D;
			default:
				break;
			}

			return VK_IMAGE_TYPE_MAX_ENUM;
		}

		VkImageViewType TextureTypeToVkVkImageViewType(TextureType textureType)
		{
			switch (textureType)
			{
			case HBL2::TextureType::D1:
				return VK_IMAGE_VIEW_TYPE_1D;
			case HBL2::TextureType::D2:
				return VK_IMAGE_VIEW_TYPE_2D;
			case HBL2::TextureType::D3:
				return VK_IMAGE_VIEW_TYPE_3D;
			default:
				break;
			}

			return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		}

		VkFormat FormatToVkFormat(Format format)
		{
			switch (format)
			{
			case Format::D32_FLOAT:
				return VK_FORMAT_D32_SFLOAT;
			case Format::RGBA8_RGB:
				return VK_FORMAT_R8G8B8A8_SRGB;
			}

			return VK_FORMAT_MAX_ENUM;
		}

		VkImageAspectFlags TextureAspectToVkImageAspectFlags(TextureAspect textureAspect)
		{
			switch (textureAspect)
			{
			case TextureAspect::NONE:
				return VK_IMAGE_ASPECT_NONE;
			case TextureAspect::COLOR:
				return VK_IMAGE_ASPECT_COLOR_BIT;
			case TextureAspect::DEPTH:
				return VK_IMAGE_ASPECT_DEPTH_BIT;
			case TextureAspect::STENCIL:
				return VK_IMAGE_ASPECT_STENCIL_BIT;
			}

			return VK_IMAGE_ASPECT_FLAG_BITS_MAX_ENUM;
		}

		VkImageUsageFlags TextureUsageToVkImageLayout(TextureUsage textureUsage)
		{
			switch (textureUsage)
			{
			case HBL2::TextureUsage::COPY_SRC:
				return VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			case HBL2::TextureUsage::COPY_DST:
				return VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			case HBL2::TextureUsage::TEXTURE_BINDING:
				return VK_ACCESS_INPUT_ATTACHMENT_READ_BIT; // TODO: Investigate if its the corrent enum
			case HBL2::TextureUsage::STORAGE_BINDING:
				return VK_IMAGE_USAGE_STORAGE_BIT;
			case HBL2::TextureUsage::RENDER_ATTACHMENT:
				return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			case HBL2::TextureUsage::SAMPLED:
				return VK_IMAGE_USAGE_SAMPLED_BIT;
			case HBL2::TextureUsage::DEPTH_STENCIL:
				return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			default:
				break;
			}

			return VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
		}
	};
}