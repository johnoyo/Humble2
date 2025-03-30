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

			// TODO: Use BitFlags here!
			VkImageUsageFlags usage = VkUtils::TextureUsageToVkImageUsageFlags(desc.usage);

			if ((desc.initialData == nullptr && Extent.width == 1 && Extent.height == 1) || desc.initialData != nullptr)
			{
				usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}
			else
			{
				if (desc.usage != TextureUsage::DEPTH_STENCIL)
				{
					usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
				}
				/*else
				{
					usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
				}*/
			}

			// Allocate Image
			VkImageCreateInfo imageCreateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
				.pNext = nullptr,
				.imageType = VkUtils::TextureTypeToVkImageType(desc.type),
				.format = VkUtils::FormatToVkFormat(desc.format),
				.extent = Extent,
				.mipLevels = 1,
				.arrayLayers = 1,
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
					.layerCount = 1,
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

		void Update(const Span<const std::byte>& bytes)
		{
			VulkanDevice* device = (VulkanDevice*)Device::Instance;
			VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

			VkBuffer stagingBuffer = VK_NULL_HANDLE;
			VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

			CreateStagingBuffer(renderer, &stagingBuffer, &stagingBufferAllocation);

			VkDeviceSize imageSize = Extent.width * Extent.height * 4;

			// Transfer initiaData to staging buffer
			void* mappedData;
			vmaMapMemory(renderer->GetAllocator(), stagingBufferAllocation, &mappedData);
			memcpy(mappedData, bytes.Data(), (size_t)imageSize);
			vmaUnmapMemory(renderer->GetAllocator(), stagingBufferAllocation);

			CopyBufferToTexture(renderer, stagingBuffer);

			vmaDestroyBuffer(renderer->GetAllocator(), stagingBuffer, stagingBufferAllocation);
		}

		void TrasitionLayout(TextureLayout currentLayout, TextureLayout newLayout)
		{
			VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;

			renderer->ImmediateSubmit([&](VkCommandBuffer cmd)
			{
				VkImageMemoryBarrier depthToShaderBarrier{};
				depthToShaderBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				depthToShaderBarrier.oldLayout = VkUtils::TextureLayoutToVkImageLayout(currentLayout);
				depthToShaderBarrier.newLayout = VkUtils::TextureLayoutToVkImageLayout(newLayout);
				depthToShaderBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				depthToShaderBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				depthToShaderBarrier.image = Image;
				depthToShaderBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				depthToShaderBarrier.subresourceRange.baseMipLevel = 0;
				depthToShaderBarrier.subresourceRange.levelCount = 1;
				depthToShaderBarrier.subresourceRange.baseArrayLayer = 0;
				depthToShaderBarrier.subresourceRange.layerCount = 1;
				depthToShaderBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				depthToShaderBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier(
					cmd,
					VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					0,
					0, nullptr,
					0, nullptr,
					1, &depthToShaderBarrier
				);
			});
		}

		void Destroy()
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

		const char* DebugName = "";
		VkImage Image = VK_NULL_HANDLE;
		VkImageView ImageView = VK_NULL_HANDLE;
		VmaAllocation Allocation = VK_NULL_HANDLE;
		VkSampler Sampler = VK_NULL_HANDLE;
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

		void CopyBufferToTexture(VulkanRenderer* renderer, VkBuffer stagingBuffer)
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
	};
}