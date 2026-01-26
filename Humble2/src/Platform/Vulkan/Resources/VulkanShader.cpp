#include "VulkanShader.h"

#include "Platform\Vulkan\VulkanResourceManager.h"
#include "Platform\Vulkan\Resources\VulkanRenderPass.h"

namespace HBL2
{
	static thread_local VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
	static thread_local VkShaderModule VertexShaderModule = VK_NULL_HANDLE;
	static thread_local VkShaderModule FragmentShaderModule = VK_NULL_HANDLE;
	static thread_local VkShaderModule ComputeShaderModule = VK_NULL_HANDLE;

	VulkanShader::VulkanShader(const ShaderDescriptor&& desc)
	{
		Recompile(std::forward<const ShaderDescriptor>(desc));
	}

	VkPipeline VulkanShader::GetOrCreateVariant(uint64_t variantHash, Handle<Material> materialHandle)
	{
		VkPipeline pipeline = s_PipelineCache.GetPipeline(variantHash);

		if (pipeline != VK_NULL_HANDLE)
		{
			return pipeline;
		}

		Material* mat = ResourceManager::Instance->GetMaterial(materialHandle);
		return GetOrCreateVariant(mat->VariantDescriptor);
	}

	VkPipeline VulkanShader::GetOrCreateVariant(const ShaderDescriptor::RenderPipeline::Variant& variantDesc)
	{
		if (!s_PipelineCache.ContainsPipeline(variantDesc))
		{
			HBL2_CORE_WARN("Vulkan Pipeline Cache does not contain key! Loading from scratch which might cause stutters!");
		}

		if (ComputeShaderModule == VK_NULL_HANDLE)
		{
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

		return s_PipelineCache.GetOrCreateComputePipeline({
			.shaderModules = { ComputeShaderModule, VK_NULL_HANDLE },
			.entryPoints = { "main", "" },
			.shaderModuleCount = 1,
			.variantDesc = variantDesc,
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

			if (desc.type == ShaderType::RASTERIZATION)
			{
				s_PipelineCache.GetOrCreatePipeline(pipelineConfig, removeVariants);
			}
			else
			{
				s_PipelineCache.GetOrCreateComputePipeline(pipelineConfig, removeVariants);
			}
		}
	}

	void VulkanShader::Destroy()
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;

		vkDestroyShaderModule(device->Get(), VertexShaderModule, nullptr);
		vkDestroyShaderModule(device->Get(), FragmentShaderModule, nullptr);
		vkDestroyShaderModule(device->Get(), ComputeShaderModule, nullptr);

		vkDestroyPipelineLayout(device->Get(), PipelineLayout, nullptr);
	}

	void VulkanShader::DestroyOld()
	{
		VulkanDevice* device = (VulkanDevice*)Device::Instance;

		vkDestroyShaderModule(device->Get(), m_OldVertexShaderModule, nullptr);
		vkDestroyShaderModule(device->Get(), m_OldFragmentShaderModule, nullptr);
		vkDestroyShaderModule(device->Get(), m_OldComputeShaderModule, nullptr);

		vkDestroyPipelineLayout(device->Get(), m_OldPipelineLayout, nullptr);
	}

	PipelineCache& VulkanShader::GetPipelineCache()
	{
		return s_PipelineCache;
	}
}
