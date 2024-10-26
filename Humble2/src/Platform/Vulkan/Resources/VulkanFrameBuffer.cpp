#include "VulkanFrameBuffer.h"

#include "Platform\Vulkan\VulkanResourceManager.h"

namespace HBL2
{
	void VulkanFrameBuffer::Create()
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

		std::vector<VkImageView> attachments;

		for (const auto& colorTarget : ColorTargets)
		{
			if (colorTarget.IsValid())
			{
				VulkanTexture* colorTexture = rm->GetTexture(colorTarget);
				attachments.push_back(colorTexture->ImageView);
			}
		}

		if (DepthTarget.IsValid())
		{
			VulkanTexture* depthTexture = rm->GetTexture(DepthTarget);
			attachments.push_back(depthTexture->ImageView);
		}

		if (!RenderPass.IsValid())
		{
			HBL2_CORE_ERROR("RenderPass handle provided in framebuffer: {}, is invalid.", DebugName);
			return;
		}

		VulkanRenderPass* vkRenderPass = rm->GetRenderPass(RenderPass);

		VkFramebufferCreateInfo frameBufferInfo =
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.renderPass = vkRenderPass->RenderPass,
			.attachmentCount = (uint32_t)attachments.size(),
			.pAttachments = attachments.data(),
			.width = Width,
			.height = Height,
			.layers = 1,
		};

		VK_VALIDATE(vkCreateFramebuffer(device->Get(), &frameBufferInfo, nullptr, &FrameBuffer), "vkCreateFramebuffer");
	}
}
