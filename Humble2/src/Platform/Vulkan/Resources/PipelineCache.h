#pragma once

#include "Resources/TypeDescriptors.h"
#include "Utilities/Collections/HashMap.h"

#include "Platform/Vulkan/VulkanCommon.h"

#include <cstring>
#include <stdint.h>

namespace HBL2
{
	struct PipelineConfig
	{
		VkShaderModule shaderModules[2];
		const char* entryPoints[2];
		uint32_t shaderModuleCount = 2;
		ShaderDescriptor::RenderPipeline::Variant variantDesc{};
		VkPipelineLayout pipelineLayout;
		VkRenderPass renderPass;
		Span<const ShaderDescriptor::RenderPipeline::VertexBufferBinding> vertexBufferBindings;
	};

	class PipelineCache
	{
	public:
		VkPipeline GetOrCreatePipeline(const PipelineConfig& config);
		VkPipeline GetPipeline(const ShaderDescriptor::RenderPipeline::Variant& variantDesc);
		void Destroy();

	private:
		VkPipeline CreatePipeline(const PipelineConfig& config);

		HashMap<ShaderDescriptor::RenderPipeline::Variant, VkPipeline> m_PipelineCache;
	};
}