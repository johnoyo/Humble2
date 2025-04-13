#include "PipelineCache.h"

#include "Platform/Vulkan/VulkanDevice.h"
#include "Platform/Vulkan/VulkanRenderer.h"
#include "Platform/Vulkan/VulkanResourceManager.h"

#include "Utilities/ShaderUtilities.h"

namespace HBL2
{
	VkPipeline PipelineCache::GetOrCreatePipeline(const PipelineConfig& config)
	{
		VkPipeline outPipeline = VK_NULL_HANDLE;

		if (m_PipelineCache.Find(config.variantDesc, outPipeline))
		{
			return outPipeline;
		}

		outPipeline = CreatePipeline(config);
		m_PipelineCache[config.variantDesc] = outPipeline;
		return outPipeline;
	}

	VkPipeline PipelineCache::GetPipeline(const ShaderDescriptor::RenderPipeline::Variant& variantDesc)
	{
		return m_PipelineCache[variantDesc];
	}

	void PipelineCache::Destroy()
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;

		for (const auto& [variantDesc, pipeline] : m_PipelineCache)
		{
			vkDestroyPipeline(device->Get(), pipeline, nullptr);
		}
	}

	VkPipeline PipelineCache::CreatePipeline(const PipelineConfig& config)
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

		// Shader stages.
		VkPipelineShaderStageCreateInfo shaderStages[2] = 
		{
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.module = config.shaderModules[0],
				.pName = config.entryPoints[0],
			},
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = config.shaderModules[1],
				.pName = config.entryPoints[1],
			}
		};

		// Vertex input state.
		std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions(config.vertexBufferBindings.Size());
		std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;

		for (uint32_t i = 0; i < config.vertexBufferBindings.Size(); i++)
		{
			const auto& binding = config.vertexBufferBindings[i];

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

		// Input assembly state.
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.pNext = nullptr,
			.topology = VkUtils::TopologyToVkPrimitiveTopology(config.variantDesc.topology),
			.primitiveRestartEnable = VK_FALSE,
		};

		// Viewport state.
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

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.viewportCount = 1,
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor,
		};

		// Rasterization state.
		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VkUtils::PolygonModeToVkPolygonMode(config.variantDesc.polygonMode),
			.cullMode = VkUtils::CullModeToVkCullModeFlags(config.variantDesc.cullMode),
			.frontFace = VkUtils::FrontFaceToVkFrontFace(config.variantDesc.frontFace),
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f,
			.lineWidth = 1.0f,
		};

		// Multisample state.
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

		// Depth Stencil state.
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.pNext = nullptr,
			.depthTestEnable = config.variantDesc.depthTest.enabled ? VK_TRUE : VK_FALSE,
			.depthWriteEnable = config.variantDesc.depthTest.writeEnabled ? VK_TRUE : VK_FALSE,
			.depthCompareOp = config.variantDesc.depthTest.enabled ? VkUtils::CompareToVkCompareOp(config.variantDesc.depthTest.depthTest) : VK_COMPARE_OP_ALWAYS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = config.variantDesc.depthTest.stencilEnabled ? VK_TRUE : VK_FALSE,
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f,
		};

		// Color blend state.
		VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
		uint32_t colorWriteMask = 0;

		if (config.variantDesc.blend.colorOutput)
		{
			colorWriteMask = VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_A_BIT;
		}

		if (config.variantDesc.blend.enabled)
		{
			colorBlendAttachmentState =
			{
				.blendEnable = VK_TRUE,
				.srcColorBlendFactor = VkUtils::BlendFactorToVkBlendFactor(config.variantDesc.blend.srcColorFactor),
				.dstColorBlendFactor = VkUtils::BlendFactorToVkBlendFactor(config.variantDesc.blend.dstColorFactor),
				.colorBlendOp = VkUtils::BlendOperationToVkBlendOp(config.variantDesc.blend.colorOp),
				.srcAlphaBlendFactor = VkUtils::BlendFactorToVkBlendFactor(config.variantDesc.blend.srcAlphaFactor),
				.dstAlphaBlendFactor = VkUtils::BlendFactorToVkBlendFactor(config.variantDesc.blend.dstAlphaFactor),
				.alphaBlendOp = VkUtils::BlendOperationToVkBlendOp(config.variantDesc.blend.alphaOp),
				.colorWriteMask = colorWriteMask,
			};
		}
		else
		{
			colorBlendAttachmentState =
			{
				.blendEnable = VK_FALSE,
				.colorWriteMask = colorWriteMask,
			};
		}

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.pNext = nullptr,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachmentState,
		};

		// Dynamic states.
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

		// Pipeline description.
		VkGraphicsPipelineCreateInfo pipelineCreateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.stageCount = config.shaderModuleCount,
			.pStages = shaderStages,
			.pVertexInputState = &vertexInputStateCreateInfo,
			.pInputAssemblyState = &inputAssemblyStateCreateInfo,
			.pViewportState = &viewportStateCreateInfo,
			.pRasterizationState = &rasterizationStateCreateInfo,
			.pMultisampleState = &multisampleStateCreateInfo,
			.pDepthStencilState = &depthStencilStateCreateInfo,
			.pColorBlendState = &colorBlendStateCreateInfo,
			.pDynamicState = &dynamicStateInfo,
			.layout = config.pipelineLayout,
			.renderPass = config.renderPass,
			.subpass = 0,
			.basePipelineHandle = VK_NULL_HANDLE,
		};

		VkPipeline pipeline = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(device->Get(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS)
		{
			HBL2_CORE_ERROR("Failed to create pipeline of shader!");
		}

		return pipeline;
	}
}