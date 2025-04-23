#include "VulkanShader.h"

#include "Platform\Vulkan\VulkanResourceManager.h"
#include "Platform\Vulkan\Resources\VulkanRenderPass.h"

namespace HBL2
{
	VulkanShader::VulkanShader(const ShaderDescriptor&& desc)
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

		DebugName = desc.debugName;
		VertexBufferBindings = desc.renderPipeline.vertexBufferBindings;
		RenderPass = rm->GetRenderPass(desc.renderPass)->RenderPass;
#if 1
		// Vertex shader module.
		{
			VkShaderModuleCreateInfo createInfo =
			{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.pNext = nullptr,
				.codeSize = 4 * desc.VS.code.Size(),
				.pCode = reinterpret_cast<const uint32_t *>(desc.VS.code.Data()),
			};
			VK_VALIDATE(vkCreateShaderModule(device->Get(), &createInfo, nullptr, &VertexShaderModule), "vkCreateShaderModule");
		}

		// Fragment shader module.
		{
			VkShaderModuleCreateInfo createInfo =
			{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.pNext = nullptr,
				.codeSize = 4 * desc.FS.code.Size(),
				.pCode = reinterpret_cast<const uint32_t *>(desc.FS.code.Data()),
			};
			VK_VALIDATE(vkCreateShaderModule(device->Get(), &createInfo, nullptr, &FragmentShaderModule), "vkCreateShaderModule");
		}

		// Pipeline layout.
		std::vector<VkDescriptorSetLayout> setLayouts;

		for (const auto& bindGroup : desc.bindGroups)
		{
			if (bindGroup.IsValid())
			{
				VulkanBindGroupLayout* vkBindGroupLayout = rm->GetBindGroupLayout(bindGroup);
				setLayouts.push_back(vkBindGroupLayout->DescriptorSetLayout);
			}
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

		VK_VALIDATE(vkCreatePipelineLayout(device->Get(), &pipelineLayoutCreateInfo, nullptr, &PipelineLayout), "vkCreatePipelineLayout");

		// Create shader variants.
		for (const auto& variant : desc.renderPipeline.variants)
		{
			s_PipelineCache.GetOrCreatePipeline( {
				.shaderModules = { VertexShaderModule, FragmentShaderModule },
				.entryPoints = { desc.VS.entryPoint, desc.FS.entryPoint },
				.shaderModuleCount = 2,
				.variantDesc = variant,
				.pipelineLayout = PipelineLayout,
				.renderPass = RenderPass,
				.vertexBufferBindings = VertexBufferBindings,
			});
		}
#else

		VkPipelineShaderStageCreateInfo shaderStages[2]{};
		shaderStages[0] = CreateShaderStageInfo(VK_SHADER_STAGE_VERTEX_BIT, VertexShaderModule, desc.VS);
		shaderStages[1] = CreateShaderStageInfo(VK_SHADER_STAGE_FRAGMENT_BIT, FragmentShaderModule, desc.FS);

		std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions(desc.renderPipeline.vertexBufferBindings.size());
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = CreateVertexInputStateCreateInfo(vertexInputBindingDescriptions, vertexInputAttributeDescriptions);

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = CreateInputAssemblyStateCreateInfo(desc.renderPipeline.topology);

		VkViewport viewport =
		{
			.x = 0.0f,
			.y = 0.0f,
			.width = (float)renderer->GetSwapchainExtent().width,
			.height = (float)renderer->GetSwapchainExtent().height,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};

		VkRect2D scissor =
		{
			.offset = { 0, 0 },
			.extent = renderer->GetSwapchainExtent(),
		};

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = CreateViewportStateCreateInfo(viewport, scissor);
		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = CreateRasterizationStateCreateInfo(desc.renderPipeline.polygonMode, desc.renderPipeline.cullMode, desc.renderPipeline.frontFace);
		VkPipelineColorBlendAttachmentState colorBlendAttachment = CreateColorBlendAttachmentState(desc.renderPipeline.blend);
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = CreateMultisampleStateCreateInfo();
		VkPipelineDepthStencilStateCreateInfo depthStencil = DepthStencilCreateInfo(desc.renderPipeline.depthTest);

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = CreateColorBlendStateCreateInfo(colorBlendAttachment);

		VkDynamicState dynamicStates[] =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};

		VkPipelineDynamicStateCreateInfo dynamicStateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = 2,
			.pDynamicStates = dynamicStates,
		};

		std::vector<VkDescriptorSetLayout> setLayouts;

		for (const auto& bindGroup : desc.bindGroups)
		{
			if (bindGroup.IsValid())
			{
				VulkanBindGroupLayout* vkBindGroupLayout = rm->GetBindGroupLayout(bindGroup);
				setLayouts.push_back(vkBindGroupLayout->DescriptorSetLayout);
			}
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

		VK_VALIDATE(vkCreatePipelineLayout(device->Get(), &pipelineLayoutCreateInfo, nullptr, &PipelineLayout), "vkCreatePipelineLayout");

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
			.pDynamicState = &dynamicStateInfo,
			.layout = PipelineLayout,
			.renderPass = RenderPass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
		};

		if (vkCreateGraphicsPipelines(device->Get(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &Pipeline) != VK_SUCCESS)
		{
			HBL2_CORE_ERROR("Failed to create pipeline of shader: {}.", DebugName);
		}
#endif
	}

	VkPipeline VulkanShader::GetOrCreateVariant(const ShaderDescriptor::RenderPipeline::Variant& variantDesc)
	{
		if (!s_PipelineCache.ContainsPipeline(variantDesc))
		{
			HBL2_CORE_WARN("Vulkan Pipeline Cache does not contain key! Loading from scratch which might cause stutters!");
		}

		return s_PipelineCache.GetOrCreatePipeline({
			.shaderModules = { VertexShaderModule, FragmentShaderModule },
			.entryPoints = { "main", "main" },
			.shaderModuleCount = 2,
			.variantDesc = variantDesc,
			.pipelineLayout = PipelineLayout,
			.renderPass = RenderPass,
			.vertexBufferBindings = VertexBufferBindings,
		});
	}

	VkPipelineShaderStageCreateInfo VulkanShader::CreateShaderStageInfo(VkShaderStageFlagBits shaderStage, VkShaderModule& shaderModule, const ShaderDescriptor::ShaderStage& stage)
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;

		VkShaderModuleCreateInfo createInfo =
		{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.pNext = nullptr,
			.codeSize = 4 * stage.code.Size(),
			.pCode = reinterpret_cast<const uint32_t*>(stage.code.Data()),
		};

		VK_VALIDATE(vkCreateShaderModule(device->Get(), &createInfo, nullptr, &shaderModule), "vkCreateShaderModule");

		VkPipelineShaderStageCreateInfo shaderStageCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.stage = shaderStage,
			.module = shaderModule,
			.pName = stage.entryPoint,
		};

		return shaderStageCreateInfo;
	}

	void VulkanShader::GetVertexDescription(std::vector<VkVertexInputBindingDescription>& vertexInputBindingDescriptions, std::vector<VkVertexInputAttributeDescription>& vertexInputAttributeDescriptions)
	{
		for (uint32_t i = 0; i < VertexBufferBindings.size(); i++)
		{
			const auto& binding = VertexBufferBindings[i];

			vertexInputBindingDescriptions[i] =
			{
				.binding = i,
				.stride = binding.byteStride,
				.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
			};

			vertexInputAttributeDescriptions.resize(binding.attributes.size());

			for (uint32_t j = 0; j < binding.attributes.size(); j++)
			{
				auto& attribute = binding.attributes[j];

				vertexInputAttributeDescriptions[j] =
				{
					.location = j,
					.binding = i,
					.format = VkUtils::VertexFormatToVkFormat(attribute.format),
					.offset = attribute.byteOffset,
				};
			}
		}
	}

	VkPipelineVertexInputStateCreateInfo VulkanShader::CreateVertexInputStateCreateInfo(std::vector<VkVertexInputBindingDescription>& vertexInputBindingDescriptions, std::vector<VkVertexInputAttributeDescription>& vertexInputAttributeDescriptions)
	{
		GetVertexDescription(vertexInputBindingDescriptions, vertexInputAttributeDescriptions);

		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.vertexBindingDescriptionCount = (uint32_t)vertexInputBindingDescriptions.size(),
			.pVertexBindingDescriptions = vertexInputBindingDescriptions.data(),
			.vertexAttributeDescriptionCount = (uint32_t)vertexInputAttributeDescriptions.size(),
			.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data(),
		};

		return vertexInputStateCreateInfo;
	}

	VkPipelineInputAssemblyStateCreateInfo VulkanShader::CreateInputAssemblyStateCreateInfo(Topology topology)
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = nullptr,
			.topology = VkUtils::TopologyToVkPrimitiveTopology(topology),
			.primitiveRestartEnable = VK_FALSE,
		};

		return inputAssemblyStateCreateInfo;
	}

	VkPipelineRasterizationStateCreateInfo VulkanShader::CreateRasterizationStateCreateInfo(PolygonMode polygonMode, CullMode cullMode, FrontFace frontFace)
	{
		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VkUtils::PolygonModeToVkPolygonMode(polygonMode),
			.cullMode = VkUtils::CullModeToVkCullModeFlags(cullMode),
			.frontFace = VkUtils::FrontFaceToVkFrontFace(frontFace),
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f,
			.lineWidth = 1.0f,
		};

		return rasterizationStateCreateInfo;
	}

	VkPipelineMultisampleStateCreateInfo VulkanShader::CreateMultisampleStateCreateInfo()
	{
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 1.0f,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE,
		};

		return multisampleStateCreateInfo;
	}

	VkPipelineColorBlendAttachmentState VulkanShader::CreateColorBlendAttachmentState(const ShaderDescriptor::RenderPipeline::BlendState& blend)
	{
		uint32_t colorWriteMask = 0;

		if (blend.colorOutput)
		{
			colorWriteMask = VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_A_BIT;
		}

		if (blend.enabled)
		{
			VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
			{
				.blendEnable = VK_TRUE,
				.srcColorBlendFactor = VkUtils::BlendFactorToVkBlendFactor(blend.srcColorFactor),
				.dstColorBlendFactor = VkUtils::BlendFactorToVkBlendFactor(blend.dstColorFactor),
				.colorBlendOp = VkUtils::BlendOperationToVkBlendOp(blend.colorOp),
				.srcAlphaBlendFactor = VkUtils::BlendFactorToVkBlendFactor(blend.srcAlphaFactor),
				.dstAlphaBlendFactor = VkUtils::BlendFactorToVkBlendFactor(blend.dstAlphaFactor),
				.alphaBlendOp = VkUtils::BlendOperationToVkBlendOp(blend.alphaOp),
				.colorWriteMask = colorWriteMask,
			};

			return colorBlendAttachmentState;
		}

		VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
		{
			.blendEnable = VK_FALSE,
			.colorWriteMask = colorWriteMask,
		};

		return colorBlendAttachmentState;
	}

	VkPipelineViewportStateCreateInfo VulkanShader::CreateViewportStateCreateInfo(VkViewport& viewport, VkRect2D& scissor)
	{
		VkPipelineViewportStateCreateInfo viewportStateCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.viewportCount = 1,
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor,
		};

		return viewportStateCreateInfo;
	}

	VkPipelineColorBlendStateCreateInfo VulkanShader::CreateColorBlendStateCreateInfo(const VkPipelineColorBlendAttachmentState& colorBlendAttachment)
	{
		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachment,
		};

		return colorBlendStateCreateInfo;
	}

	VkPipelineDepthStencilStateCreateInfo VulkanShader::DepthStencilCreateInfo(const ShaderDescriptor::RenderPipeline::DepthTest& depthTest)
	{
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = nullptr,
			.depthTestEnable = depthTest.enabled ? VK_TRUE : VK_FALSE,
			.depthWriteEnable = depthTest.writeEnabled ? VK_TRUE : VK_FALSE,
			.depthCompareOp = depthTest.enabled ? VkUtils::CompareToVkCompareOp(depthTest.depthTest) : VK_COMPARE_OP_ALWAYS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = depthTest.stencilEnabled ? VK_TRUE : VK_FALSE,
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f,
		};

		return depthStencilStateCreateInfo;
	}
}
