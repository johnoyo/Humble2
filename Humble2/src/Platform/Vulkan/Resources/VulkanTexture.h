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
		VulkanTexture(const TextureDescriptor&& desc);

		void Update(const Span<const std::byte>& bytes);

		void TrasitionLayout(TextureLayout currentLayout, TextureLayout newLayout, PipelineStage srcStage, PipelineStage dstStage);

		void Destroy();

		const char* DebugName = "";
		VkImage Image = VK_NULL_HANDLE;
		VkImageView ImageView = VK_NULL_HANDLE;
		VmaAllocation Allocation = VK_NULL_HANDLE;
		VkSampler Sampler = VK_NULL_HANDLE;
		VkExtent3D Extent{};
		TextureType Type = TextureType::D2;

		VkImageAspectFlags Aspect = 0;

	private:
		void CreateStagingBuffer(VulkanRenderer* renderer, VkBuffer* stagingBuffer, VmaAllocation* stagingBufferAllocation);

		void CopyBufferToTexture(VulkanRenderer* renderer, VkBuffer stagingBuffer);
	};
}