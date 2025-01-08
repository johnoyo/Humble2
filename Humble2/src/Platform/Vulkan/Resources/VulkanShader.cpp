#include "VulkanShader.h"

#include "Platform\Vulkan\VulkanResourceManager.h"
#include "Platform\Vulkan\Resources\VulkanRenderPass.h"

namespace HBL2
{
	VulkanShader::VulkanShader(const ShaderDescriptor&& desc)
	{
		Device = (VulkanDevice*)Device::Instance;
		Renderer = (VulkanRenderer*)Renderer::Instance;
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

		DebugName = desc.debugName;
		VertexBufferBindings = desc.renderPipeline.vertexBufferBindings;

		VkPipelineShaderStageCreateInfo shaderStages[2]{};
		shaderStages[0] = CreateShaderStageInfo(VK_SHADER_STAGE_VERTEX_BIT, VertexShaderModule, desc.VS);
		shaderStages[1] = CreateShaderStageInfo(VK_SHADER_STAGE_FRAGMENT_BIT, FragmentShaderModule, desc.FS);

		std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions(VertexBufferBindings.size());
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = CreateVertexInputStateCreateInfo(vertexInputBindingDescriptions, vertexInputAttributeDescriptions);

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = CreateInputAssemblyStateCreateInfo(desc.renderPipeline.topology);

		VkViewport viewport =
		{
			.x = 0.0f,
			.y = 0.0f,
			.width = (float)Renderer->GetSwapchainExtent().width,
			.height = (float)Renderer->GetSwapchainExtent().height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};

		VkRect2D scissor =
		{
			.offset = { 0, 0 },
			.extent = Renderer->GetSwapchainExtent(),
		};

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = CreateViewportStateCreateInfo(viewport, scissor);

		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = CreateRasterizationStateCreateInfo(desc.renderPipeline.polygonMode, desc.renderPipeline.cullMode, desc.renderPipeline.frontFace);
		VkPipelineColorBlendAttachmentState colorBlendAttachment = CreateColorBlendAttachmentState(desc.renderPipeline.blend);
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = CreateMultisampleStateCreateInfo();
		VkPipelineDepthStencilStateCreateInfo depthStencil = DepthStencilCreateInfo(desc.renderPipeline.depthTest);

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = CreateColorBlendStateCreateInfo(colorBlendAttachment);

		std::vector<VkDescriptorSetLayout> setLayouts;

		for (const auto& bindGroup : desc.bindGroups)
		{
			VulkanBindGroupLayout* vkBindGroupLayout = rm->GetBindGroupLayout(bindGroup);
			setLayouts.push_back(vkBindGroupLayout->DescriptorSetLayout);
		}

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.setLayoutCount = (uint32_t)setLayouts.size(),
			.pSetLayouts = setLayouts.data(),
			.pushConstantRangeCount = 0,
			.pPushConstantRanges = nullptr,
		};

		VK_VALIDATE(vkCreatePipelineLayout(Device->Get(), &pipelineLayoutCreateInfo, nullptr, &PipelineLayout), "vkCreatePipelineLayout")

		VulkanRenderPass* vkRenderPass = rm->GetRenderPass(desc.renderPass);

		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.stageCount = 2,
			.pStages = shaderStages,
			.pVertexInputState = &vertexInputStateCreateInfo,
			.pInputAssemblyState = &inputAssemblyCreateInfo,
			.pViewportState = &viewportStateCreateInfo,
			.pRasterizationState = &rasterizationStateCreateInfo,
			.pMultisampleState = &multisampleStateCreateInfo,
			.pDepthStencilState = &depthStencil,
			.pColorBlendState = &colorBlendStateCreateInfo,
			.layout = PipelineLayout,
			.renderPass = vkRenderPass->RenderPass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
		};

		if (vkCreateGraphicsPipelines(Device->Get(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &Pipeline) != VK_SUCCESS)
		{
			HBL2_CORE_ERROR("Failed to create pipeline of shader: {}.", DebugName);
		}

		vkDestroyShaderModule(Device->Get(), VertexShaderModule, nullptr);
		vkDestroyShaderModule(Device->Get(), FragmentShaderModule, nullptr);
	}
}
