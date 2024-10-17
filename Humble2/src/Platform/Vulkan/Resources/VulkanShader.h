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
		
		const char* DebugName = "";
		VkPipeline Pipeline = VK_NULL_HANDLE;
		VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
		VkShaderModule VertexShaderModule = VK_NULL_HANDLE;
		VkShaderModule FragmentShaderModule = VK_NULL_HANDLE;

		VulkanDevice* Device = nullptr;
		VulkanRenderer* Renderer = nullptr;
		std::vector<ShaderDescriptor::RenderPipeline::VertexBufferBinding> VertexBufferBindings;

	private:
		VkPipelineShaderStageCreateInfo CreateShaderStageInfo(VkShaderStageFlagBits shaderStage, const ShaderDescriptor::ShaderStage& stage)
		{
			VkShaderModuleCreateInfo createInfo =
			{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.pNext = nullptr,
				.codeSize = stage.code.size(),
				.pCode = reinterpret_cast<const uint32_t*>(stage.code.data()),
			};

			VkShaderModule shaderModule;
			VK_VALIDATE(vkCreateShaderModule(Device->Get(), &createInfo, nullptr, &shaderModule), "vkCreateShaderModule");

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

		void GetVertexDescription(std::vector<VkVertexInputBindingDescription>& vertexInputBindingDescriptions, std::vector<VkVertexInputAttributeDescription>& vertexInputAttributeDescriptions)
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

				vertexInputAttributeDescriptions.reserve(binding.attributes.size());

				for (uint32_t j = 0; j < binding.attributes.size(); j++)
				{
					auto& attribute = binding.attributes[j];

					vertexInputAttributeDescriptions[j] =
					{
						.location = j,
						.binding = i,
						.format = VertexFormatToVkFormat(attribute.format),
						.offset = attribute.byteOffset,
					};
				}
			}
		}

		VkPipelineVertexInputStateCreateInfo CreateVertexInputStateCreateInfo()
		{
			std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions(VertexBufferBindings.size());
			std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;

			GetVertexDescription(vertexInputBindingDescriptions, vertexInputAttributeDescriptions);

			VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
				.pNext = nullptr,
				.vertexBindingDescriptionCount = (uint32_t)vertexInputBindingDescriptions.size(),
				.pVertexBindingDescriptions = vertexInputBindingDescriptions.data(),
				.vertexAttributeDescriptionCount = (uint32_t)vertexInputAttributeDescriptions.size(),
				.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data(),
			};

			return vertexInputStateCreateInfo;
		}

		VkPipelineInputAssemblyStateCreateInfo CreateInputAssemblyStateCreateInfo(VkPrimitiveTopology topology)
		{
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
				.pNext = nullptr,
				.topology = topology,
				.primitiveRestartEnable = VK_FALSE,
			};

			return inputAssemblyStateCreateInfo;
		}

		VkPipelineRasterizationStateCreateInfo CreateRasterizationStateCreateInfo(VkPolygonMode polygonMode)
		{
			VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.pNext = nullptr,
				.depthClampEnable = VK_FALSE,
				.rasterizerDiscardEnable = VK_FALSE,
				.polygonMode = polygonMode,
				.cullMode = VK_CULL_MODE_NONE,
				.frontFace = VK_FRONT_FACE_CLOCKWISE,
				.depthBiasEnable = VK_FALSE,
				.depthBiasConstantFactor = 0.0f,
				.depthBiasClamp = 0.0f,
				.depthBiasSlopeFactor = 0.0f,
				.lineWidth = 1.0f,
			};			

			return rasterizationStateCreateInfo;
		}

		VkPipelineMultisampleStateCreateInfo CreateMultisampleStateCreateInfo()
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

		VkPipelineColorBlendAttachmentState CreateColorBlendAttachmentState(const ShaderDescriptor::RenderPipeline::BlendState& blend)
		{
			if (blend.enabled)
			{
				// TODO: Fill fields with info from blend
				VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
				{
					.blendEnable = VK_TRUE,
					.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
				};

				return colorBlendAttachmentState;
			}

			VkPipelineColorBlendAttachmentState colorBlendAttachmentState =
			{
				.blendEnable = VK_FALSE,
				.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
			};

			return colorBlendAttachmentState;
		}

		VkPipelineDepthStencilStateCreateInfo DepthStencilCreateInfo(const ShaderDescriptor::RenderPipeline::DepthTest& depthTest)
		{
			VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
				.pNext = nullptr,
				.depthTestEnable = depthTest.enabled ? VK_TRUE : VK_FALSE,
				.depthWriteEnable = depthTest.writeEnabled ? VK_TRUE : VK_FALSE,
				.depthCompareOp = depthTest.enabled ? VK_COMPARE_OP_LESS : VK_COMPARE_OP_ALWAYS, // TODO: get compare op from depthTest
				.depthBoundsTestEnable = VK_FALSE,
				.stencilTestEnable = depthTest.stencilEnabled ? VK_TRUE : VK_FALSE,
				.minDepthBounds = 0.0f,
				.maxDepthBounds = 1.0f,
			};

			return depthStencilStateCreateInfo;
		}

		VkPipelineViewportStateCreateInfo CreateViewportStateCreateInfo()
		{
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

		VkPipelineColorBlendStateCreateInfo CreateColorBlendStateCreateInfo(const VkPipelineColorBlendAttachmentState& colorBlendAttachment)
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

		VkPipelineLayoutCreateInfo CreatePipelineLayoutCreateInfo()
		{
			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.setLayoutCount = 0,
				.pSetLayouts = nullptr,
				.pushConstantRangeCount = 0,
				.pPushConstantRanges = nullptr,
			};

			return pipelineLayoutCreateInfo;
		}

		VkFormat VertexFormatToVkFormat(VertexFormat vertexFormat)
		{
			switch (vertexFormat)
			{
			case HBL2::VertexFormat::FLOAT32:
				return VK_FORMAT_R32_SFLOAT;
			case HBL2::VertexFormat::FLOAT32x2:
				return VK_FORMAT_R32G32_SFLOAT;
			case HBL2::VertexFormat::FLOAT32x3:
				return VK_FORMAT_R32G32B32_SFLOAT;
			case HBL2::VertexFormat::FLOAT32x4:
				return VK_FORMAT_R32G32B32A32_SFLOAT;
			case HBL2::VertexFormat::INT32:
				return VK_FORMAT_R32_SINT;
			case HBL2::VertexFormat::INT32x2:
				return VK_FORMAT_R32G32_SINT;
			case HBL2::VertexFormat::INT32x3:
				return VK_FORMAT_R32G32B32_SINT;
			case HBL2::VertexFormat::INT32x4:
				return VK_FORMAT_R32G32B32A32_SINT;
			case HBL2::VertexFormat::UINT32:
				return VK_FORMAT_R32_UINT;
			case HBL2::VertexFormat::UINT32x2:
				return VK_FORMAT_R32G32_UINT;
			case HBL2::VertexFormat::UINT32x3:
				return VK_FORMAT_R32G32B32_UINT;
			case HBL2::VertexFormat::UINT32x4:
				return VK_FORMAT_R32G32B32A32_UINT;
			}

			return VK_FORMAT_MAX_ENUM;
		}
	};
}