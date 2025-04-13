#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanDevice.h"
#include "Platform\Vulkan\VulkanRenderer.h"

#include "Platform\Vulkan\VulkanCommon.h"
#include "Platform\Vulkan\Resources\PipelineCache.h"

namespace HBL2
{
	struct VulkanShader
	{
		VulkanShader() = default;
		VulkanShader(const ShaderDescriptor&& desc);

		void AddVariant(const ShaderDescriptor::RenderPipeline::Variant& variantDesc);
		VkPipeline GetVariantPipeline(const ShaderDescriptor::RenderPipeline::Variant& variantDesc);

		void Destroy()
		{
			VulkanDevice* device = (VulkanDevice*)Device::Instance;

			vkDestroyShaderModule(device->Get(), VertexShaderModule, nullptr);
			vkDestroyShaderModule(device->Get(), FragmentShaderModule, nullptr);

			vkDestroyPipelineLayout(device->Get(), PipelineLayout, nullptr);
			vkDestroyPipeline(device->Get(), Pipeline, nullptr);
		}

		static PipelineCache& GetPipelineCache()
		{
			return s_PipelineCache;
		}

		const char* DebugName = "";
		VkPipeline Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
		VkRenderPass RenderPass = VK_NULL_HANDLE;
		VkShaderModule VertexShaderModule = VK_NULL_HANDLE;
		VkShaderModule FragmentShaderModule = VK_NULL_HANDLE;
		std::vector<ShaderDescriptor::RenderPipeline::VertexBufferBinding> VertexBufferBindings;

	private:
		static inline PipelineCache s_PipelineCache{};

		VkPipelineShaderStageCreateInfo CreateShaderStageInfo(VkShaderStageFlagBits shaderStage, VkShaderModule& shaderModule, const ShaderDescriptor::ShaderStage& stage);

		void GetVertexDescription(std::vector<VkVertexInputBindingDescription>& vertexInputBindingDescriptions, std::vector<VkVertexInputAttributeDescription>& vertexInputAttributeDescriptions);

		VkPipelineVertexInputStateCreateInfo CreateVertexInputStateCreateInfo(std::vector<VkVertexInputBindingDescription>& vertexInputBindingDescriptions, std::vector<VkVertexInputAttributeDescription>& vertexInputAttributeDescriptions);

		VkPipelineInputAssemblyStateCreateInfo CreateInputAssemblyStateCreateInfo(Topology topology);

		VkPipelineRasterizationStateCreateInfo CreateRasterizationStateCreateInfo(PolygonMode polygonMode, CullMode cullMode, FrontFace frontFace);

		VkPipelineMultisampleStateCreateInfo CreateMultisampleStateCreateInfo();

		VkPipelineColorBlendAttachmentState CreateColorBlendAttachmentState(const ShaderDescriptor::RenderPipeline::BlendState& blend);

		VkPipelineViewportStateCreateInfo CreateViewportStateCreateInfo(VkViewport& viewport, VkRect2D& scissor);

		VkPipelineColorBlendStateCreateInfo CreateColorBlendStateCreateInfo(const VkPipelineColorBlendAttachmentState& colorBlendAttachment);

		VkPipelineDepthStencilStateCreateInfo DepthStencilCreateInfo(const ShaderDescriptor::RenderPipeline::DepthTest& depthTest);
	};
}