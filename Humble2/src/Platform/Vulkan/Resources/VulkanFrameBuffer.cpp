#include "VulkanFrameBuffer.h"

#include "Platform\Vulkan\VulkanResourceManager.h"

namespace HBL2
{
	VulkanFrameBuffer::VulkanFrameBuffer(const FrameBufferDescriptor&& desc)
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

		DebugName = desc.debugName;

		std::vector<VkImageView> attachments;

		for (const auto& colorTarget : desc.colorTargets)
		{
			if (colorTarget.IsValid())
			{
				VulkanTexture* colorTexture = rm->GetTexture(colorTarget);
				attachments.push_back(colorTexture->ImageView);
			}
		}

		if (desc.depthTarget.IsValid())
		{
			VulkanTexture* depthTexture = rm->GetTexture(desc.depthTarget);
			attachments.push_back(depthTexture->ImageView);
		}

		if (!desc.renderPass.IsValid())
		{
			HBL2_CORE_ERROR("RenderPass handle provided in framebuffer: {}, is invalid.", DebugName);
			return;
		}

		VulkanRenderPass* vkRenderPass = rm->GetRenderPass(desc.renderPass);

		VkFramebufferCreateInfo frameBufferInfo =
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = nullptr,
			.renderPass = vkRenderPass->RenderPass,
			.attachmentCount = (uint32_t)attachments.size(),
			.pAttachments = attachments.data(),
			.width = desc.width,
			.height = desc.height,
			.layers = 1,
		};

		VK_VALIDATE(vkCreateFramebuffer(device->Get(), &frameBufferInfo, nullptr, &FrameBuffer), "vkCreateFramebuffer");
	}
}
