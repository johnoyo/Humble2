#include "VulkanRenderPass.h"

#include "Platform/Vulkan/VulkanResourceManager.h"

namespace HBL2
{
	VulkanRenderPass::VulkanRenderPass(const RenderPassDescriptor&& desc)
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

		DebugName = desc.debugName;
		StencilClearValue = desc.depthTarget.stencilLoadOp == LoadOperation::CLEAR;
		ClearStencil = desc.depthTarget.clearStencil;

		VulkanRenderPassLayout* layout = rm->GetRenderPassLayout(desc.layout);

		std::vector<VkAttachmentDescription> attachments;
		attachments.reserve(desc.colorTargets.Size());
		std::vector<VkAttachmentReference> colorAttachmentRefs;
		colorAttachmentRefs.reserve(desc.colorTargets.Size());

		uint32_t index = 0;

		for (const auto& colorTarget : desc.colorTargets)
		{
			ClearColor = colorTarget.clearColor;
			ColorClearValues.push_back(colorTarget.loadOp == LoadOperation::CLEAR);

			attachments.push_back(VkAttachmentDescription
			{
				.format = VkUtils::FormatToVkFormat(colorTarget.format),
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VkUtils::LoadOperationToVkAttachmentLoadOp(colorTarget.loadOp),
				.storeOp = VkUtils::StoreOperationVkAttachmentStoreOp(colorTarget.storeOp),
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VkUtils::TextureLayoutToVkImageLayout(colorTarget.prevUsage),
				.finalLayout = VkUtils::TextureLayoutToVkImageLayout(colorTarget.nextUsage),
			});

			colorAttachmentRefs.push_back(VkAttachmentReference
			{
				.attachment = index++,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			});
		}

		std::vector<VkSubpassDependency> dependencies;

		if (desc.colorTargets.Size() != 0)
		{
			VkSubpassDependency colorDependency =
			{
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			};

			dependencies.push_back(colorDependency);
		}

		VkAttachmentReference depthAttachmentRef;

		for (const auto& subpass : layout->SubPasses)
		{
			if (subpass.depthTarget)
			{
				DepthClearValue = desc.depthTarget.loadOp == LoadOperation::CLEAR;
				ClearDepth = desc.depthTarget.clearZ;

				attachments.push_back(VkAttachmentDescription
				{
					.flags = 0,
					.format = VkUtils::FormatToVkFormat(layout->DepthTargetFormat),
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.loadOp = VkUtils::LoadOperationToVkAttachmentLoadOp(desc.depthTarget.loadOp),
					.storeOp = VkUtils::StoreOperationVkAttachmentStoreOp(desc.depthTarget.storeOp),
					.stencilLoadOp = VkUtils::LoadOperationToVkAttachmentLoadOp(desc.depthTarget.stencilLoadOp),
					.stencilStoreOp = VkUtils::StoreOperationVkAttachmentStoreOp(desc.depthTarget.stencilStoreOp),
					.initialLayout = VkUtils::TextureLayoutToVkImageLayout(desc.depthTarget.prevUsage),
					.finalLayout = VkUtils::TextureLayoutToVkImageLayout(desc.depthTarget.nextUsage),
				});

				depthAttachmentRef =
				{
					.attachment = index++,
					.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				};

				VkSubpassDependency depthDependency =
				{
					.srcSubpass = VK_SUBPASS_EXTERNAL,
					.dstSubpass = 0,
					.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					.srcAccessMask = 0,
					.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				};

				dependencies.push_back(depthDependency);
			}
		}

		//we are going to create 1 subpass, which is the minimum you can do
		VkSubpassDescription subpass =
		{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = (uint32_t)colorAttachmentRefs.size(),
			.pColorAttachments = colorAttachmentRefs.data(),
			.pDepthStencilAttachment = &depthAttachmentRef,
		};

		VkRenderPassCreateInfo renderPassInfo =
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = (uint32_t)attachments.size(),
			.pAttachments = attachments.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = (uint32_t)dependencies.size(),
			.pDependencies = dependencies.data(),
		};		

		VK_VALIDATE(vkCreateRenderPass(device->Get(), &renderPassInfo, nullptr, &RenderPass), "vkCreateRenderPass");
        
        // Abort frame buffer creation if width or height are 0.
        if (desc.frameBufferDesc.width == 0 || desc.frameBufferDesc.height == 0)
        {
            return;
        }
        
        CreateFrameBuffer(std::forward<const FrameBufferDescriptor>(desc.frameBufferDesc));
	}

    void VulkanRenderPass::CreateFrameBuffer(const FrameBufferDescriptor&& desc)
    {
        Width = desc.width;
        Height = desc.height;
        ColorTargets = desc.colorTargets;
        DepthTarget = desc.depthTarget;
        
        VulkanDevice* device = (VulkanDevice*)Device::Instance;
        VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

        if (FrameBuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(device->Get(), FrameBuffer, nullptr);
        }

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

        if (RenderPass == VK_NULL_HANDLE)
        {
            HBL2_CORE_ERROR("RenderPass handle provided in framebuffer: {}, is invalid.", DebugName);
            return;
        }

        VkFramebufferCreateInfo frameBufferInfo =
        {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .renderPass = RenderPass,
            .attachmentCount = (uint32_t)attachments.size(),
            .pAttachments = attachments.data(),
            .width = Width,
            .height = Height,
            .layers = 1,
        };

        VK_VALIDATE(vkCreateFramebuffer(device->Get(), &frameBufferInfo, nullptr, &FrameBuffer), "vkCreateFramebuffer");
    }
}
