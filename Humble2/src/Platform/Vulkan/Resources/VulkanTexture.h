#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "VulkanBindGroup.h"

#include "Platform\Vulkan\VulkanDevice.h"
#include "Platform\Vulkan\VulkanRenderer.h"

#include "Platform\Vulkan\VulkanCommon.h"

namespace HBL2
{
	struct VulkanTexture
	{
		VulkanTexture() = default;
		VulkanTexture(const TextureDescriptor&& desc);

		void Update(const Span<const std::byte>& bytes);
		void TrasitionLayout(VulkanCommandBuffer* commandBuffer, TextureLayout currentLayout, TextureLayout newLayout, VulkanBindGroup* bindGroup);
		void Destroy();

		const char* DebugName = "";
		VkImage Image = VK_NULL_HANDLE;
		VkImageView ImageView = VK_NULL_HANDLE;
		VmaAllocation Allocation = VK_NULL_HANDLE;
		VkSampler Sampler = VK_NULL_HANDLE;
		VkExtent3D Extent{};
		VkImageLayout ImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		TextureType ImageType = TextureType::D2;

		VkImageAspectFlags Aspect = 0;

	private:
		void CreateStagingBuffer(VulkanRenderer* renderer, VkBuffer* stagingBuffer, VmaAllocation* stagingBufferAllocation);
		void CopyBufferToTexture(VulkanRenderer* renderer, VkBuffer stagingBuffer);

		friend class Pool<VulkanTexture, Texture>; // This is required for a hack to create the swapchain images in the VulkanRenderer
		VulkanTexture(const VulkanTexture&& other);
	private:
		uint32_t m_PixelByteSize = 0;
	};
}