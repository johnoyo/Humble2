#include "VulkanShader.h"

#include "Platform\Vulkan\VulkanResourceManager.h"
#include "Platform\Vulkan\Resources\VulkanRenderPass.h"

namespace HBL2
{
	VulkanShader::VulkanShader(const ShaderDescriptor&& desc)
	{
		Recompile(std::forward<const ShaderDescriptor>(desc));
	}

	VkPipeline VulkanShader::Find(ShaderDescriptor::RenderPipeline::PackedVariant key, uint32_t* pipelineIndex, bool forceCreateNewAndRemoveOld)
	{
		uint32_t n = m_Count.load(std::memory_order_acquire);

		for (uint32_t i = 0; i < n; i++)
		{
			if (m_Entries[i].Key == key)
			{
				*pipelineIndex = i;
				return m_Entries[i].Pipeline;				
			}
		}

		return VK_NULL_HANDLE;
	}

	VkPipeline VulkanShader::GetOrCreateVariant(ShaderDescriptor::RenderPipeline::PackedVariant key)
	{
		return GetOrCreatePipeline({
			.shaderModules = { VertexShaderModule, FragmentShaderModule },
			.entryPoints = { "main", "main" },
			.shaderModuleCount = 2,
			.variantDesc = key,
			.pipelineLayout = PipelineLayout,
			.renderPass = RenderPass,
			.vertexBufferBindings = VertexBufferBindings,
		});
	}

	VkPipeline VulkanShader::GetOrCreateComputeVariant(ShaderDescriptor::RenderPipeline::PackedVariant key)
	{
		return GetOrCreatePipeline({
			.shaderModules = { ComputeShaderModule, VK_NULL_HANDLE },
			.entryPoints = { "main", "" },
			.shaderModuleCount = 1,
			.variantDesc = key,
			.pipelineLayout = PipelineLayout,
			.renderPass = RenderPass,
			.vertexBufferBindings = VertexBufferBindings,
		});
	}
	
	void VulkanShader::Recompile(const ShaderDescriptor&& desc, bool removeVariants)
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

		m_OldVertexShaderModule = VertexShaderModule;
		m_OldFragmentShaderModule = FragmentShaderModule;
		m_OldComputeShaderModule = ComputeShaderModule;
		m_OldPipelineLayout = PipelineLayout;

		DebugName = desc.debugName;
		VertexBufferBindings = desc.renderPipeline.vertexBufferBindings;
		RenderPass = rm->GetRenderPass(desc.renderPass)->RenderPass;

		StaticArray<VkShaderModule, 2> shaderModules{};
		StaticArray<const char*, 2> entryPoints{};
		uint32_t shaderModuleCount = 0;

		if (desc.type == ShaderType::RASTERIZATION)
		{
			// Vertex shader module.
			{
				VkShaderModuleCreateInfo createInfo =
				{
					.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
					.pNext = nullptr,
					.codeSize = 4 * desc.VS.code.Size(),
					.pCode = reinterpret_cast<const uint32_t*>(desc.VS.code.Data()),
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
					.pCode = reinterpret_cast<const uint32_t*>(desc.FS.code.Data()),
				};
				VK_VALIDATE(vkCreateShaderModule(device->Get(), &createInfo, nullptr, &FragmentShaderModule), "vkCreateShaderModule");
			}

			entryPoints[0] = desc.VS.entryPoint;
			shaderModules[0] = VertexShaderModule;
			entryPoints[1] = desc.FS.entryPoint;
			shaderModules[1] = FragmentShaderModule;
			shaderModuleCount = 2;
		}
		else
		{
			VkShaderModuleCreateInfo createInfo =
			{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.pNext = nullptr,
				.codeSize = 4 * desc.CS.code.Size(),
				.pCode = reinterpret_cast<const uint32_t*>(desc.CS.code.Data()),
			};
			VK_VALIDATE(vkCreateShaderModule(device->Get(), &createInfo, nullptr, &ComputeShaderModule), "vkCreateShaderModule");

			entryPoints[0] = desc.CS.entryPoint;
			shaderModules[0] = ComputeShaderModule;
			entryPoints[1] = "";
			shaderModules[1] = VK_NULL_HANDLE;
			shaderModuleCount = 1;
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

		if (!setLayouts.empty())
		{
			PipelineLayoutHash = (uint64_t)setLayouts[0];
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
			const PipelineConfig pipelineConfig =
			{
				.shaderModules = shaderModules,
				.entryPoints = entryPoints,
				.shaderModuleCount = shaderModuleCount,
				.variantDesc = variant,
				.pipelineLayout = PipelineLayout,
				.renderPass = RenderPass,
				.vertexBufferBindings = VertexBufferBindings,
			};

			GetOrCreatePipeline(pipelineConfig, removeVariants);
		}
	}

	void VulkanShader::Destroy()
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;

		vkDestroyShaderModule(device->Get(), VertexShaderModule, nullptr);
		vkDestroyShaderModule(device->Get(), FragmentShaderModule, nullptr);
		vkDestroyShaderModule(device->Get(), ComputeShaderModule, nullptr);

		vkDestroyPipelineLayout(device->Get(), PipelineLayout, nullptr);

		for (const auto& variantEntry : m_Entries)
		{
			vkDestroyPipeline(device->Get(), variantEntry.Pipeline, nullptr);
		}

		for (const auto& pipeline : m_RetiredPipelines)
		{
			vkDestroyPipeline(device->Get(), pipeline, nullptr);
		}
	}

	void VulkanShader::DestroyOld()
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;

		vkDestroyShaderModule(device->Get(), m_OldVertexShaderModule, nullptr);
		vkDestroyShaderModule(device->Get(), m_OldFragmentShaderModule, nullptr);
		vkDestroyShaderModule(device->Get(), m_OldComputeShaderModule, nullptr);

		vkDestroyPipelineLayout(device->Get(), m_OldPipelineLayout, nullptr);
	}

	VkPipeline VulkanShader::GetOrCreatePipeline(const PipelineConfig& config, bool forceCreateNewAndRemoveOld)
	{
		VkPipeline p = VK_NULL_HANDLE;
		uint32_t pipelineIndex = UINT32_MAX;

		// Fast path with a lock-free read.
		if (p = Find(config.variantDesc, &pipelineIndex, forceCreateNewAndRemoveOld); p != VK_NULL_HANDLE && !forceCreateNewAndRemoveOld)
		{
			return p;
		}

		// Slow path where only writers lock, if the pipeline is not in cache.
		std::lock_guard lock(m_WriteMutex);

		// Re-check after acquiring lock (another thread may have inserted).
		if (p == VK_NULL_HANDLE)
		{
			if (p = Find(config.variantDesc, &pipelineIndex, forceCreateNewAndRemoveOld); p != VK_NULL_HANDLE && !forceCreateNewAndRemoveOld)
			{
				return p;
			}
		}

		// If we got a hit in the cache and we want to force create new and remove old, we append the pipeline to the retired array.
		if (forceCreateNewAndRemoveOld && p != VK_NULL_HANDLE)
		{
			// Get current number of entries.
			uint32_t n = m_Count.load(std::memory_order_relaxed);

			// Swap entry to remove with last element.
			const uint32_t last = n - 1;

			if (pipelineIndex != last)
			{
				m_Entries[pipelineIndex] = m_Entries[last];
			}

			// Invalidate key and pipeline.
			m_Entries[last].Key = g_NullVariant;
			m_Entries[last].Pipeline = VK_NULL_HANDLE;

			// Update number of entries.
			m_Count.store(last, std::memory_order_release);

			// Append pipeline to retired array for cleanup.
			m_RetiredPipelines.push_back(p);
		}

		// Create pipeline
		if (ComputeShaderModule == VK_NULL_HANDLE)
		{
			p = CreatePipeline(config);
		}
		else
		{
			p = CreateComputePipeline(config);
		}

		// Check if we exceeded max variant count.
		uint32_t idx = m_Count.load(std::memory_order_relaxed);

		if (idx >= MaxVariants)
		{
			return p;
		}

		// Store in array and update count.
		m_Entries[idx] = { config.variantDesc, p };
		m_Count.store(idx + 1, std::memory_order_release);

		return p;
	}

	VkPipeline VulkanShader::CreatePipeline(const PipelineConfig& config)
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
			.topology = VkUtils::TopologyToVkPrimitiveTopology((Topology)config.variantDesc.topology),
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
			.polygonMode = VkUtils::PolygonModeToVkPolygonMode((PolygonMode)config.variantDesc.polygonMode),
			.cullMode = VkUtils::CullModeToVkCullModeFlags((CullMode)config.variantDesc.cullMode),
			.frontFace = VkUtils::FrontFaceToVkFrontFace((FrontFace)config.variantDesc.frontFace),
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 0.0f,
			.depthBiasClamp = 0.0f,
			.depthBiasSlopeFactor = 0.0f,
			.lineWidth = 1.5f,
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
			.depthTestEnable = config.variantDesc.depthEnabled ? VK_TRUE : VK_FALSE,
			.depthWriteEnable = config.variantDesc.depthWrite ? VK_TRUE : VK_FALSE,
			.depthCompareOp = config.variantDesc.depthEnabled ? VkUtils::CompareToVkCompareOp((Compare)config.variantDesc.depthCompare) : VK_COMPARE_OP_ALWAYS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = config.variantDesc.stencilEnabled ? VK_TRUE : VK_FALSE,
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f,
		};

		// Color blend state.
		VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
		uint32_t colorWriteMask = 0;

		if (config.variantDesc.colorOutput)
		{
			colorWriteMask = VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_A_BIT;
		}

		if (config.variantDesc.blendEnabled)
		{
			colorBlendAttachmentState =
			{
				.blendEnable = VK_TRUE,
				.srcColorBlendFactor = VkUtils::BlendFactorToVkBlendFactor((BlendFactor)config.variantDesc.srcColorFactor),
				.dstColorBlendFactor = VkUtils::BlendFactorToVkBlendFactor((BlendFactor)config.variantDesc.dstColorFactor),
				.colorBlendOp = VkUtils::BlendOperationToVkBlendOp((BlendOperation)config.variantDesc.colorOp),
				.srcAlphaBlendFactor = VkUtils::BlendFactorToVkBlendFactor((BlendFactor)config.variantDesc.srcAlphaFactor),
				.dstAlphaBlendFactor = VkUtils::BlendFactorToVkBlendFactor((BlendFactor)config.variantDesc.dstAlphaFactor),
				.alphaBlendOp = VkUtils::BlendOperationToVkBlendOp((BlendOperation)config.variantDesc.alphaOp),
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

	VkPipeline VulkanShader::CreateComputePipeline(const PipelineConfig& config)
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;

		VkPipelineShaderStageCreateInfo stageInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = config.shaderModules[0],
			.pName = "main",
		};

		VkComputePipelineCreateInfo pipelineInfo =
		{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.pNext = nullptr,
			.stage = stageInfo,
			.layout = config.pipelineLayout
		};

		VkPipeline computePipeline = VK_NULL_HANDLE;

		if (vkCreateComputePipelines(device->Get(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS)
		{
			HBL2_CORE_ERROR("Failed to create pipeline of compute shader!");
		}

		return computePipeline;
	}
}
