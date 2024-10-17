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

			VkImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.pNext = nullptr;
			imageCreateInfo.imageType = TextureTypeToVkImageType(desc.type);
			imageCreateInfo.format = FormatToVkFormat(desc.format);
			imageCreateInfo.extent = Extent;
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.usage = TextureUsageToVkImageLayout(desc.usage);

			VmaAllocationCreateInfo allocationCreateInfo = {};
			allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			allocationCreateInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_VALIDATE(vmaCreateImage(renderer->GetAllocator(), &imageCreateInfo, &allocationCreateInfo, &Image, &Allocation, nullptr), "vmaCreateImage");

			VkImageViewCreateInfo imageViewCreateInfo = {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.pNext = nullptr;
			imageViewCreateInfo.viewType = TextureTypeToVkVkImageViewType(desc.type);
			imageViewCreateInfo.image = Image;
			imageViewCreateInfo.format = FormatToVkFormat(desc.internalFormat);
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // TODO: Fix!

			VK_VALIDATE(vkCreateImageView(device->Get(), &imageViewCreateInfo, nullptr, &ImageView), "vkCreateImageView");
		}

		const char* DebugName = "";
		VkImage Image = VK_NULL_HANDLE;
		VkImageView ImageView = VK_NULL_HANDLE;
		VmaAllocation Allocation = VK_NULL_HANDLE;
		VkExtent3D Extent{};

	private:
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
			}

			return VK_FORMAT_MAX_ENUM;
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