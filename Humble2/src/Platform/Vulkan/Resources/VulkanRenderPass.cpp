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
		attachments.reserve(desc.colorTargets.Size);
		std::vector<VkAttachmentReference> colorAttachmentRefs;
		colorAttachmentRefs.reserve(desc.colorTargets.Size);

		uint32_t index = 0;

		for (const auto& colorTarget : desc.colorTargets)
		{
			attachments.push_back(VkAttachmentDescription{
				.format = renderer->GetSwapchainImageFormat(),
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = LoadOperationToVkAttachmentLoadOp(colorTarget.loadOp),
				.storeOp = StoreOperationVkAttachmentStoreOp(colorTarget.storeOp),
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = TextureLayoutToVkImageLayout(colorTarget.prevUsage),
				.finalLayout = TextureLayoutToVkImageLayout(colorTarget.nextUsage),
			});

			colorAttachmentRefs.push_back(VkAttachmentReference{
				.attachment = index++,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			});
		}

		VkAttachmentReference depthAttachmentRef;

		for (const auto& subpass : layout->SubPasses)
		{
			if (subpass.depthTarget)
			{
				attachments.push_back(VkAttachmentDescription{
					.flags = 0,
					.format = FormatToVkFormat(layout->DepthTargetFormat),
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.loadOp = LoadOperationToVkAttachmentLoadOp(desc.depthTarget.loadOp),
					.storeOp = StoreOperationVkAttachmentStoreOp(desc.depthTarget.storeOp),
					.stencilLoadOp = LoadOperationToVkAttachmentLoadOp(desc.depthTarget.stencilLoadOp),
					.stencilStoreOp = StoreOperationVkAttachmentStoreOp(desc.depthTarget.stencilStoreOp),
					.initialLayout = TextureLayoutToVkImageLayout(desc.depthTarget.prevUsage),
					.finalLayout = TextureLayoutToVkImageLayout(desc.depthTarget.nextUsage),
				});

				depthAttachmentRef = {
					.attachment = index++,
					.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				};
			}
		}

		//we are going to create 1 subpass, which is the minimum you can do
		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = colorAttachmentRefs.data();
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkSubpassDependency depthDependency = {};
		depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		depthDependency.dstSubpass = 0;
		depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depthDependency.srcAccessMask = 0;
		depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkSubpassDependency dependencies[2] = { dependency, depthDependency };

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 2;
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = &dependencies[0];

		VK_VALIDATE(vkCreateRenderPass(device->Get(), &renderPassInfo, nullptr, &RenderPass), "vkCreateRenderPass");
	}
}
