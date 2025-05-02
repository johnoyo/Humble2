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

		VkPipeline GetOrCreateVariant(const ShaderDescriptor::RenderPipeline::Variant& variantDesc);
		void Destroy();
		static PipelineCache& GetPipelineCache();

		const char* DebugName = "";
		VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
		VkRenderPass RenderPass = VK_NULL_HANDLE;
		VkShaderModule VertexShaderModule = VK_NULL_HANDLE;
		VkShaderModule FragmentShaderModule = VK_NULL_HANDLE;
		VkShaderModule ComputeShaderModule = VK_NULL_HANDLE;
		std::vector<ShaderDescriptor::RenderPipeline::VertexBufferBinding> VertexBufferBindings;

	private:
		static inline PipelineCache s_PipelineCache{};
	};
}