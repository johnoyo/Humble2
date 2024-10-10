#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanDevice.h"
#include "Platform\Vulkan\VulkanRenderer.h"

#include "vulkan\vulkan.h"
#include "vma\vk_mem_alloc.h"

#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>

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
			
			switch (desc.type)
			{
			case TextureType::D1:
				ImageType = VK_IMAGE_TYPE_1D;
				ImageViewType = VK_IMAGE_VIEW_TYPE_1D;
				break;
			case TextureType::D2:
				ImageType = VK_IMAGE_TYPE_2D;
				ImageViewType = VK_IMAGE_VIEW_TYPE_2D;
				break;
			case TextureType::D3:
				ImageType = VK_IMAGE_TYPE_3D;
				ImageViewType = VK_IMAGE_VIEW_TYPE_3D;
				break;
			}

			switch (desc.format)
			{
			case Format::D32_FLOAT:
				ImageFormat = VK_FORMAT_D32_SFLOAT;
				break;
			}

			switch (desc.internalFormat)
			{
			case Format::D32_FLOAT:
				ImageViewFormat = VK_FORMAT_D32_SFLOAT;
				break;
			}

			Extent = { desc.dimensions.x, desc.dimensions.y, desc.dimensions.z };

			switch (desc.usage)
			{
			case TextureUsage::DEPTH_STENCIL:
				ImageUsageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				break;
			}

			VkImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.pNext = nullptr;
			imageCreateInfo.imageType = ImageType;
			imageCreateInfo.format = ImageFormat;
			imageCreateInfo.extent = Extent;
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.usage = ImageUsageFlags;

			VmaAllocationCreateInfo allocationCreateInfo = {};
			allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			allocationCreateInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			VK_VALIDATE(vmaCreateImage(renderer->GetAllocator(), &imageCreateInfo, &allocationCreateInfo, &Image, &Allocation, nullptr), "vmaCreateImage");

			VkImageViewCreateInfo imageViewCreateInfo = {};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.pNext = nullptr;
			imageViewCreateInfo.viewType = ImageViewType;
			imageViewCreateInfo.image = Image;
			imageViewCreateInfo.format = ImageViewFormat;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // TODO: Fix!

			VK_VALIDATE(vkCreateImageView(device->Get(), &imageViewCreateInfo, nullptr, &ImageView), "vkCreateImageView");
		}

		const char* DebugName = "";
		VkImage Image;
		VkImageView ImageView;
		VmaAllocation Allocation;
		VkExtent3D Extent;
		VkImageType ImageType;
		VkImageViewType ImageViewType;
		VkFormat ImageFormat;
		VkImageUsageFlags ImageUsageFlags;
		VkFormat ImageViewFormat;
	};
}