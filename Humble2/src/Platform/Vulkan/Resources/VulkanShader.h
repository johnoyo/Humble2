#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanDevice.h"
#include "Platform\Vulkan\VulkanRenderer.h"

#include "Platform\Vulkan\VulkanCommon.h"

namespace HBL2
{
	struct VulkanShader
	{
		VulkanShader() = default;
		VulkanShader(const ShaderDescriptor&& desc);
		
		void Destroy()
		{
			VulkanDevice* device = (VulkanDevice*)Device::Instance;

			vkDestroyPipelineLayout(device->Get(), PipelineLayout, nullptr);
			vkDestroyPipeline(device->Get(), Pipeline, nullptr);
		}

		const char* DebugName = "";
		VkPipeline Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
		VkShaderModule VertexShaderModule = VK_NULL_HANDLE;
		VkShaderModule FragmentShaderModule = VK_NULL_HANDLE;

		VulkanDevice* Device = nullptr;
		VulkanRenderer* Renderer = nullptr;
		std::vector<ShaderDescriptor::RenderPipeline::VertexBufferBinding> VertexBufferBindings;

	private:
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