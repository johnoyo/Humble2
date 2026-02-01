#include "VulkanShader.h"

#include "Platform\Vulkan\VulkanResourceManager.h"
#include "Platform\Vulkan\Resources\VulkanRenderPass.h"

namespace HBL2
{
	void VulkanShaderHot::Destroy()
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		vkDestroyPipelineLayout(device->Get(), PipelineLayout, nullptr);
	}

	VkPipeline VulkanShaderCold::Find(ShaderDescriptor::RenderPipeline::PackedVariant key, uint32_t* pipelineIndex, bool forceCreateNewAndRemoveOld)
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

	void VulkanShaderCold::Destroy()
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;

		vkDestroyShaderModule(device->Get(), VertexShaderModule, nullptr);
		vkDestroyShaderModule(device->Get(), FragmentShaderModule, nullptr);
		vkDestroyShaderModule(device->Get(), ComputeShaderModule, nullptr);

		for (const auto& variantEntry : m_Entries)
		{
			vkDestroyPipeline(device->Get(), variantEntry.Pipeline, nullptr);
		}

		for (const auto& pipeline : m_RetiredPipelines)
		{
			vkDestroyPipeline(device->Get(), pipeline, nullptr);
		}
	}

	void VulkanShaderCold::DestroyOld()
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;

		vkDestroyShaderModule(device->Get(), m_OldVertexShaderModule, nullptr);
		vkDestroyShaderModule(device->Get(), m_OldFragmentShaderModule, nullptr);
		vkDestroyShaderModule(device->Get(), m_OldComputeShaderModule, nullptr);

		vkDestroyPipelineLayout(device->Get(), m_OldPipelineLayout, nullptr);
	}

	VkPipeline VulkanShaderCold::GetOrCreatePipeline(const PipelineConfig& config, bool forceCreateNewAndRemoveOld)
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

	VkPipeline VulkanShaderCold::CreatePipeline(const PipelineConfig& config)
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

	VkPipeline VulkanShaderCold::CreateComputePipeline(const PipelineConfig& config)
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

	bool VulkanShader::IsValid() const
	{
		return Cold != nullptr && Hot != nullptr;
	}

	void VulkanShader::Initialize(const ShaderDescriptor&& desc)
	{
		Recompile(std::forward<const ShaderDescriptor>(desc));
	}

	VkPipeline VulkanShader::GetOrCreateVariant(ShaderDescriptor::RenderPipeline::PackedVariant key)
	{
		if (!IsValid())
		{
			return VK_NULL_HANDLE;
		}

		return Cold->GetOrCreatePipeline({
			.shaderModules = { Cold->VertexShaderModule, Cold->FragmentShaderModule },
			.entryPoints = { "main", "main" },
			.shaderModuleCount = 2,
			.variantDesc = key,
			.pipelineLayout = Hot->PipelineLayout,
			.renderPass = Cold->RenderPass,
			.vertexBufferBindings = Cold->VertexBufferBindings,
		});
	}

	VkPipeline VulkanShader::GetOrCreateComputeVariant(ShaderDescriptor::RenderPipeline::PackedVariant key)
	{
		if (!IsValid())
		{
			return VK_NULL_HANDLE;
		}

		return Cold->GetOrCreatePipeline({
			.shaderModules = { Cold->ComputeShaderModule, VK_NULL_HANDLE },
			.entryPoints = { "main", "" },
			.shaderModuleCount = 1,
			.variantDesc = key,
			.pipelineLayout = Hot->PipelineLayout,
			.renderPass = Cold->RenderPass,
			.vertexBufferBindings = Cold->VertexBufferBindings,
		});
	}
	
	void VulkanShader::Recompile(const ShaderDescriptor&& desc, bool removeVariants)
	{
		if (!IsValid())
		{
			return;
		}

		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanRenderer* renderer = (VulkanRenderer*)Renderer::Instance;
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

		Cold->m_OldVertexShaderModule = Cold->VertexShaderModule;
		Cold->m_OldFragmentShaderModule = Cold->FragmentShaderModule;
		Cold->m_OldComputeShaderModule = Cold->ComputeShaderModule;
		Cold->m_OldPipelineLayout = Hot->PipelineLayout;

		Cold->DebugName = desc.debugName;
		Cold->VertexBufferBindings = desc.renderPipeline.vertexBufferBindings;
		Cold->RenderPass = rm->GetRenderPass(desc.renderPass)->RenderPass;

		std::array<VkShaderModule, 2> shaderModules{};
		std::array<const char*, 2> entryPoints{};
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
				VK_VALIDATE(vkCreateShaderModule(device->Get(), &createInfo, nullptr, &Cold->VertexShaderModule), "vkCreateShaderModule");
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
				VK_VALIDATE(vkCreateShaderModule(device->Get(), &createInfo, nullptr, &Cold->FragmentShaderModule), "vkCreateShaderModule");
			}

			entryPoints[0] = desc.VS.entryPoint;
			shaderModules[0] = Cold->VertexShaderModule;
			entryPoints[1] = desc.FS.entryPoint;
			shaderModules[1] = Cold->FragmentShaderModule;
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
			VK_VALIDATE(vkCreateShaderModule(device->Get(), &createInfo, nullptr, &Cold->ComputeShaderModule), "vkCreateShaderModule");

			entryPoints[0] = desc.CS.entryPoint;
			shaderModules[0] = Cold->ComputeShaderModule;
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
			Hot->PipelineLayoutHash = (uint64_t)setLayouts[0];
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

		VK_VALIDATE(vkCreatePipelineLayout(device->Get(), &pipelineLayoutCreateInfo, nullptr, &Hot->PipelineLayout), "vkCreatePipelineLayout");

		// Create shader variants.
		for (const auto& variant : desc.renderPipeline.variants)
		{
			const VulkanShaderCold::PipelineConfig pipelineConfig =
			{
				.shaderModules = shaderModules,
				.entryPoints = entryPoints,
				.shaderModuleCount = shaderModuleCount,
				.variantDesc = variant,
				.pipelineLayout = Hot->PipelineLayout,
				.renderPass = Cold->RenderPass,
				.vertexBufferBindings = Cold->VertexBufferBindings,
			};

			Cold->GetOrCreatePipeline(pipelineConfig, removeVariants);
		}
	}

	void VulkanShader::Destroy()
	{
		if (!IsValid())
		{
			return;
		}

		Hot->Destroy();
		Cold->Destroy();
	}

	void VulkanShader::DestroyOld()
	{
		if (!IsValid())
		{
			return;
		}

		Cold->DestroyOld();
	}
}
