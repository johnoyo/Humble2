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
		StaticArray<VkShaderModule, 2> shaderModules;
		StaticArray<const char*, 2> entryPoints;
		uint32_t shaderModuleCount = 2;
		ShaderDescriptor::RenderPipeline::Variant variantDesc{};
		VkPipelineLayout pipelineLayout;
		VkRenderPass renderPass;
		Span<const ShaderDescriptor::RenderPipeline::VertexBufferBinding> vertexBufferBindings;
	};

	class PipelineCache
	{
	public:
		VkPipeline GetPipeline(uint64_t variantHash);
		VkPipeline GetOrCreatePipeline(const PipelineConfig& config, bool forceCreateNewAndRemoveOld = false);
		VkPipeline GetOrCreateComputePipeline(const PipelineConfig& config, bool forceCreateNewAndRemoveOld = false);
		bool ContainsPipeline(uint64_t variantHash);
		bool ContainsPipeline(const ShaderDescriptor::RenderPipeline::Variant& variantDesc);
		void Destroy();

	private:
		VkPipeline CreatePipeline(const PipelineConfig& config);
		VkPipeline CreateComputePipeline(const PipelineConfig& config);
		std::mutex m_CacheMutex;
		std::unordered_map<uint64_t, VkPipeline> m_PipelineCache;
		std::vector<VkPipeline> m_RetiredPipelines;
	};
}