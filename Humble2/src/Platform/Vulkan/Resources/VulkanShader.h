#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\Vulkan\VulkanDevice.h"
#include "Platform\Vulkan\VulkanRenderer.h"

#include "Platform\Vulkan\VulkanCommon.h"

namespace HBL2
{
	struct VulkanShaderHot
	{
		uint64_t PipelineLayoutHash = UINT64_MAX; // Hot
		VkPipelineLayout PipelineLayout = VK_NULL_HANDLE; // Hot

		void Destroy();
	};

	struct VulkanShaderCold
	{
		VkPipeline Find(ShaderDescriptor::RenderPipeline::PackedVariant key, uint32_t* pipelineIndex, bool forceCreateNewAndRemoveOld = false);
		void Destroy();
		void DestroyOld();

		const char* DebugName = "";

		VkRenderPass RenderPass = VK_NULL_HANDLE;
		VkShaderModule VertexShaderModule = VK_NULL_HANDLE;
		VkShaderModule FragmentShaderModule = VK_NULL_HANDLE;
		VkShaderModule ComputeShaderModule = VK_NULL_HANDLE;

		std::vector<ShaderDescriptor::RenderPipeline::VertexBufferBinding> VertexBufferBindings;

	private:
		struct PipelineConfig
		{
			std::array<VkShaderModule, 2> shaderModules;
			std::array<const char*, 2> entryPoints;
			uint32_t shaderModuleCount = 2;
			ShaderDescriptor::RenderPipeline::PackedVariant variantDesc{};
			VkPipelineLayout pipelineLayout;
			VkRenderPass renderPass;
			Span<const ShaderDescriptor::RenderPipeline::VertexBufferBinding> vertexBufferBindings;
		};

		VkPipeline GetOrCreatePipeline(const PipelineConfig& config, bool forceCreateNewAndRemoveOld = false);
		VkPipeline CreatePipeline(const PipelineConfig& config);
		VkPipeline CreateComputePipeline(const PipelineConfig& config);

		VkPipelineLayout m_OldPipelineLayout = VK_NULL_HANDLE;
		VkShaderModule m_OldVertexShaderModule = VK_NULL_HANDLE;
		VkShaderModule m_OldFragmentShaderModule = VK_NULL_HANDLE;
		VkShaderModule m_OldComputeShaderModule = VK_NULL_HANDLE;

		std::vector<VkPipeline> m_RetiredPipelines;
		std::atomic<uint32_t> m_Count{ 0 };
		mutable std::mutex m_WriteMutex;
		
		struct VariantEntry
		{
			ShaderDescriptor::RenderPipeline::PackedVariant Key = g_NullVariant;
			VkPipeline Pipeline = VK_NULL_HANDLE;
		};

		static constexpr uint32_t MaxVariants = 8;

		alignas(64) std::array<VariantEntry, MaxVariants> m_Entries;

		friend class VulkanShader;
	};

	struct VulkanShader
	{
		VulkanShader() = default;

		bool IsValid() const;

		void Initialize(const ShaderDescriptor&& desc);

		VkPipeline GetOrCreateVariant(ShaderDescriptor::RenderPipeline::PackedVariant key);
		VkPipeline GetOrCreateComputeVariant(ShaderDescriptor::RenderPipeline::PackedVariant key);

		void Recompile(const ShaderDescriptor&& desc, bool removeVariants = false);
		void Destroy();
		void DestroyOld();

		VulkanShaderHot* Hot = nullptr;
		VulkanShaderCold* Cold = nullptr;	
	};
}