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

		VkPipeline GetOrCreateVariant(uint64_t variantHash, Handle<Material> materialHandle);
		VkPipeline GetOrCreateVariant(const ShaderDescriptor::RenderPipeline::Variant& variantDesc);

		void Recompile(const ShaderDescriptor&& desc, bool removeVariants = false);
		void Destroy();
		void DestroyOld();

		static PipelineCache& GetPipelineCache();

		const char* DebugName = "";
		VkRenderPass RenderPass = VK_NULL_HANDLE;
		VkPipelineLayout PipelineLayout = VK_NULL_HANDLE; // Hot
		VkShaderModule VertexShaderModule = VK_NULL_HANDLE;
		VkShaderModule FragmentShaderModule = VK_NULL_HANDLE;
		VkShaderModule ComputeShaderModule = VK_NULL_HANDLE;

		std::vector<ShaderDescriptor::RenderPipeline::VertexBufferBinding> VertexBufferBindings;

		uint64_t PipelineLayoutHash = UINT64_MAX;

	private:
		static inline PipelineCache s_PipelineCache{};
		
		VkPipelineLayout m_OldPipelineLayout = VK_NULL_HANDLE;
		VkShaderModule m_OldVertexShaderModule = VK_NULL_HANDLE;
		VkShaderModule m_OldFragmentShaderModule = VK_NULL_HANDLE;
		VkShaderModule m_OldComputeShaderModule = VK_NULL_HANDLE;
	};
}