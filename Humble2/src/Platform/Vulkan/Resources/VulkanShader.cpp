#include "VulkanShader.h"

#include "Platform/Vulkan/VulkanResourceManager.h"
#include "Platform/Vulkan/Resources/VulkanRenderPass.h"

namespace HBL2
{
	void VulkanShaderHot::Destroy()
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		vkDestroyPipelineLayout(device->Get(), PipelineLayout, nullptr);

		ResourceManager::Instance->DeleteBindGroup(ShaderBindGroup);
	}

	VkPipeline VulkanShaderCold::Find(ShaderDescriptor::RenderPipeline::PackedVariant key, uint32_t* pipelineIndex)
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

		m_RetiredPipelines.clear();

		for (auto bindGroupLayout : m_ReflectedBindGroupLayouts)
		{
			ResourceManager::Instance->DeleteBindGroupLayout(bindGroupLayout);
		}

		m_ReflectedBindGroupLayouts.clear();
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
		if (p = Find(config.variantDesc, &pipelineIndex); p != VK_NULL_HANDLE && !forceCreateNewAndRemoveOld)
		{
			return p;
		}

		// Slow path where only writers lock, if the pipeline is not in cache.
		std::lock_guard lock(m_WriteMutex);

		// Re-check after acquiring lock (another thread may have inserted).
		if (p == VK_NULL_HANDLE)
		{
			if (p = Find(config.variantDesc, &pipelineIndex); p != VK_NULL_HANDLE && !forceCreateNewAndRemoveOld)
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

		SpecializationData vertexSpecializationData;
		BuildSpecializationInfo(ShaderStage::VERTEX, vertexSpecializationData, config);

		SpecializationData fragmentSpecializationData;
		BuildSpecializationInfo(ShaderStage::FRAGMENT, fragmentSpecializationData, config);

		// Shader stages.
		VkPipelineShaderStageCreateInfo shaderStages[2] =
		{
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.module = config.shaderModules[0],
				.pName = config.entryPoints[0],
				.pSpecializationInfo = (vertexSpecializationData.data.empty() ? NULL : &vertexSpecializationData.info),
			},
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = nullptr,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.module = config.shaderModules[1],
				.pName = config.entryPoints[1],
				.pSpecializationInfo = (fragmentSpecializationData.data.empty() ? NULL : &fragmentSpecializationData.info),
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

		SpecializationData computeSpecializationData;
		BuildSpecializationInfo(ShaderStage::COMPUTE, computeSpecializationData, config);

		VkPipelineShaderStageCreateInfo stageInfo =
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = config.shaderModules[0],
			.pName = config.entryPoints[0],
			.pSpecializationInfo = (computeSpecializationData.data.empty() ? NULL : &computeSpecializationData.info),
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

	static bool GetVariantShaderConstantValueFromIndex(const ShaderDescriptor::RenderPipeline::PackedVariant& variant, uint32_t i)
	{
		if (i == 0) { return variant.shaderConstantBool0; }
		if (i == 1) { return variant.shaderConstantBool1; }
		if (i == 2) { return variant.shaderConstantBool2; }
		if (i == 3) { return variant.shaderConstantBool3; }
		if (i == 4) { return variant.shaderConstantBool4; }
		if (i == 5) { return variant.shaderConstantBool5; }
		if (i == 6) { return variant.shaderConstantBool6; }
		if (i == 7) { return variant.shaderConstantBool7; }
        
        return false;
	}

	void VulkanShaderCold::BuildSpecializationInfo(ShaderStage stage, SpecializationData& specializationData, const PipelineConfig& config)
	{
		const auto& constantStages = config.specializationConstantStages;

		if (constantStages.Size() == 0)
		{
			return;
		}

		uint32_t offset = 0;
		uint32_t constantID = 0;

		for (uint32_t i = 0; i < constantStages.Size(); i++)
		{
			if (!constantStages[i].IsSet(stage))
			{
				constantID++;
				continue;
			}

			VkSpecializationMapEntry entry{};
			entry.constantID = constantID++;
			entry.offset = offset;

			{
				// Vulkan bool spec constants are typically uint32_t
				const uint32_t value = GetVariantShaderConstantValueFromIndex(config.variantDesc, i) ? 1u : 0u;

				const void* src = &value;
				size_t size = sizeof(value);

				entry.size = size;
				const size_t oldSize = specializationData.data.size();
				specializationData.data.resize((uint32_t)(oldSize + size));
				std::memcpy(specializationData.data.data() + oldSize, src, size);
				offset += static_cast<uint32_t>(size);
			}

			specializationData.entries.push_back(entry);
		}

		specializationData.info.mapEntryCount = static_cast<uint32_t>(specializationData.entries.size());
		specializationData.info.pMapEntries = specializationData.entries.data();
		specializationData.info.dataSize = specializationData.data.size();
		specializationData.info.pData = specializationData.data.data();
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

		if (Cold->ComputeShaderModule == VK_NULL_HANDLE)
		{
			return Cold->GetOrCreatePipeline({
				.shaderModules = { Cold->VertexShaderModule, Cold->FragmentShaderModule },
				.entryPoints = { "mainVS", "mainPS" }, // TODO: fix me!
				.shaderModuleCount = 2,
				.variantDesc = key,
				.pipelineLayout = Hot->PipelineLayout,
				.renderPass = Cold->RenderPass,
				.vertexBufferBindings = Cold->VertexBufferBindings,
				.specializationConstantStages = Cold->m_SpecializationConstantStages,
			});
		}

		return GetOrCreateComputeVariant(key);
	}

	VkPipeline VulkanShader::GetOrCreateComputeVariant(ShaderDescriptor::RenderPipeline::PackedVariant key)
	{
		if (!IsValid())
		{
			return VK_NULL_HANDLE;
		}

		return Cold->GetOrCreatePipeline({
			.shaderModules = { Cold->ComputeShaderModule, VK_NULL_HANDLE },
			.entryPoints = { "mainCS", "" }, // TODO: fix me!
			.shaderModuleCount = 1,
			.variantDesc = key,
			.pipelineLayout = Hot->PipelineLayout,
			.renderPass = Cold->RenderPass,
			.vertexBufferBindings = Cold->VertexBufferBindings,
			.specializationConstantStages = Cold->m_SpecializationConstantStages,
		});
	}
	
	static void SyncVariantWithSpecializationConstant(uint32_t i, const ShaderConstant& specializationConstant, ShaderDescriptor::RenderPipeline::PackedVariant& variant)
	{
		if (i == 0) { variant.shaderConstantBool0 = (ShaderDescriptor::RenderPipeline::packed_size)specializationConstant.value.b; return; }
		if (i == 1) { variant.shaderConstantBool1 = (ShaderDescriptor::RenderPipeline::packed_size)specializationConstant.value.b; return; }
		if (i == 2) { variant.shaderConstantBool2 = (ShaderDescriptor::RenderPipeline::packed_size)specializationConstant.value.b; return; }
		if (i == 3) { variant.shaderConstantBool3 = (ShaderDescriptor::RenderPipeline::packed_size)specializationConstant.value.b; return; }
		if (i == 4) { variant.shaderConstantBool4 = (ShaderDescriptor::RenderPipeline::packed_size)specializationConstant.value.b; return; }
		if (i == 5) { variant.shaderConstantBool5 = (ShaderDescriptor::RenderPipeline::packed_size)specializationConstant.value.b; return; }
		if (i == 6) { variant.shaderConstantBool6 = (ShaderDescriptor::RenderPipeline::packed_size)specializationConstant.value.b; return; }
		if (i == 7) { variant.shaderConstantBool7 = (ShaderDescriptor::RenderPipeline::packed_size)specializationConstant.value.b; return; }
	}

	void VulkanShader::Recompile(const ShaderDescriptor&& desc, bool removeVariants)
	{
		if (!IsValid())
		{
			return;
		}

		VulkanDevice* device = (VulkanDevice*)Device::Instance;
		VulkanResourceManager* rm = (VulkanResourceManager*)ResourceManager::Instance;

		Cold->m_OldVertexShaderModule = Cold->VertexShaderModule;
		Cold->m_OldFragmentShaderModule = Cold->FragmentShaderModule;
		Cold->m_OldComputeShaderModule = Cold->ComputeShaderModule;
		Cold->m_OldPipelineLayout = Hot->PipelineLayout;

		Cold->VertexShaderModule = VK_NULL_HANDLE;
		Cold->FragmentShaderModule = VK_NULL_HANDLE;
		Cold->ComputeShaderModule = VK_NULL_HANDLE;

		Cold->DebugName = desc.debugName;

		// BindGroups use a reference counting system, so if there are other objects
		// referencing the bindgroup, it will not be deleted, just the ref count will be decreased.
		if (Hot->ShaderBindGroup.IsValid())
		{
			ResourceManager::Instance->DeleteBindGroup(Hot->ShaderBindGroup);
		}
		Hot->ShaderBindGroup = desc.shaderBindGroup;

		// Clear VertexBufferBindings.
		Cold->VertexBufferBindings.clear();

		// Fill with new ones.
		for (auto& vbb : desc.renderPipeline.vertexBufferBindings)
		{
			Cold->VertexBufferBindings.push_back(vbb);
		}

		// Clear m_SpecializationConstantStages.
		for (uint32_t i = 0; i < Cold->m_SpecializationConstantStages.size(); i++)
		{
			Cold->m_SpecializationConstantStages[i].Clear();
		}

		// Fill with new ones.
		for (uint32_t i = 0; i < desc.renderPipeline.specializationConstantsPerVariant.Size(); i++)
		{
			for (uint32_t j = 0; j < desc.renderPipeline.specializationConstantsPerVariant[i].Size(); j++)
			{
				auto& variant = *((ShaderDescriptor::RenderPipeline::PackedVariant*)&desc.renderPipeline.variants[i]);
				const auto& specializationConstant = desc.renderPipeline.specializationConstantsPerVariant[i][j];

				SyncVariantWithSpecializationConstant(j, specializationConstant, variant);

				Cold->m_SpecializationConstantStages[j] = specializationConstant.stage;
			}
		}

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
					.codeSize = desc.VS.code.Size(),
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
					.codeSize = desc.FS.code.Size(),
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
				.codeSize = desc.CS.code.Size(),
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
		StaticDArray<VkDescriptorSetLayout, 8> setLayouts;
		uint32_t bindGroupLayoutIndex = 0;

		// Release and clear old cache reflected bind group layouts.
		for (auto bindGroupLayout : Cold->m_ReflectedBindGroupLayouts)
		{
			ResourceManager::Instance->DeleteBindGroupLayout(bindGroupLayout);
		}
		Cold->m_ReflectedBindGroupLayouts.clear();

		for (const auto& bindGroup : desc.bindGroups)
		{
			if (bindGroup.IsValid())
			{
				if (bindGroupLayoutIndex == 0)
				{
					Hot->GlobalBindGroupLayoutHash = bindGroup.HashKey();
				}

				if (bindGroupLayoutIndex == 1)
				{
					// Keep reference to the reflected bind group layout of set 1,
					// since reflection increases the ref count of the layout obj and
					// we need to release it on shader destroy for proper clean up.
					if (bindGroup != Renderer::Instance->GetEmptyBindingsLayout() && desc.bindGroups.size() == 4)
					{
						Cold->m_ReflectedBindGroupLayouts.push_back(bindGroup);
					}
				}

				if (bindGroupLayoutIndex == 2 && desc.bindGroups.size() == 4)
				{
					// Keep reference to the reflected bind group layout of set 2,
					// since reflection increases the ref count of the layout obj and
					// we need to release it on shader destroy for proper clean up.
					if (bindGroup != Renderer::Instance->GetEmptyBindingsLayout())
					{
						Cold->m_ReflectedBindGroupLayouts.push_back(bindGroup);
					}
				}

				VulkanBindGroupLayout* vkBindGroupLayout = rm->GetBindGroupLayout(bindGroup);
				setLayouts.push_back(vkBindGroupLayout->DescriptorSetLayout);
			}

			bindGroupLayoutIndex++;
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
		for (int i = 0; i < desc.renderPipeline.variants.Size(); i++)
		{
			const auto& variant = desc.renderPipeline.variants[i];

			const VulkanShaderCold::PipelineConfig pipelineConfig =
			{
				.shaderModules = shaderModules,
				.entryPoints = entryPoints,
				.shaderModuleCount = shaderModuleCount,
				.variantDesc = variant,
				.pipelineLayout = Hot->PipelineLayout,
				.renderPass = Cold->RenderPass,
				.vertexBufferBindings = Cold->VertexBufferBindings,
				.specializationConstantStages = Cold->m_SpecializationConstantStages,
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
