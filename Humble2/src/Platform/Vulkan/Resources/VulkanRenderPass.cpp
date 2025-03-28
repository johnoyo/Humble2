#include "VulkanRenderPass.h"

#include "Platform\Vulkan\VulkanResourceManager.h"

namespace HBL2
{
	VulkanRenderPass::VulkanRenderPass(const RenderPassDescriptor&& desc)
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

		DebugName = desc.debugName;

		VulkanRenderPassLayout* layout = rm->GetRenderPassLayout(desc.layout);

		std::vector<VkAttachmentDescription> attachments;
		attachments.reserve(desc.colorTargets.Size());
		std::vector<VkAttachmentReference> colorAttachmentRefs;
		colorAttachmentRefs.reserve(desc.colorTargets.Size());

		uint32_t index = 0;

		for (const auto& colorTarget : desc.colorTargets)
		{
			ColorClearValues.push_back(colorTarget.loadOp == LoadOperation::CLEAR);

			attachments.push_back(VkAttachmentDescription
			{
				.format = renderer->GetSwapchainImageFormat(),
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

		VkAttachmentReference depthAttachmentRef;

		for (const auto& subpass : layout->SubPasses)
		{
			if (subpass.depthTarget)
			{
				DepthClearValue = desc.depthTarget.loadOp == LoadOperation::CLEAR;

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
			}
		}

		//we are going to create 1 subpass, which is the minimum you can do
		VkSubpassDescription subpass =
		{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = colorAttachmentRefs.data(),
			.pDepthStencilAttachment = &depthAttachmentRef,
		};

		VkSubpassDependency dependency =
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
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

		VkSubpassDependency dependencies[2] = { dependency, depthDependency };

		VkRenderPassCreateInfo renderPassInfo =
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = 2,
			.pAttachments = attachments.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 2,
			.pDependencies = &dependencies[0],
		};		

		VK_VALIDATE(vkCreateRenderPass(device->Get(), &renderPassInfo, nullptr, &RenderPass), "vkCreateRenderPass");
	}
}
