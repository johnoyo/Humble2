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
		shaderStages[0] = CreateShaderStageInfo(VK_SHADER_STAGE_VERTEX_BIT, desc.VS);
		shaderStages[1] = CreateShaderStageInfo(VK_SHADER_STAGE_FRAGMENT_BIT, desc.FS);

		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = CreateVertexInputStateCreateInfo();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = CreateInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST); // TODO: Add topology to desc

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = CreateViewportStateCreateInfo();

		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = CreateRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL); // TODO: Add fill to desc
		VkPipelineColorBlendAttachmentState colorBlendAttachment = CreateColorBlendAttachmentState(desc.renderPipeline.blend);
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = CreateMultisampleStateCreateInfo();
		VkPipelineDepthStencilStateCreateInfo depthStencil = DepthStencilCreateInfo(desc.renderPipeline.depthTest);

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = CreateColorBlendStateCreateInfo(colorBlendAttachment);

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = CreatePipelineLayoutCreateInfo(); // TODO: Add descriptor sets to pipeline layout

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
