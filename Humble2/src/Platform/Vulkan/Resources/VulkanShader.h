#pragma once

#include "Base.h"
#include "Resources/TypeDescriptors.h"

#include "Platform/Vulkan/VulkanDevice.h"
#include "Platform/Vulkan/VulkanRenderer.h"

#include "Platform/Vulkan/VulkanCommon.h"

namespace HBL2
{
	struct VulkanShaderHot
	{
		uint32_t GlobalBindGroupLayoutHash = 0;
		Handle<BindGroup> ShaderBindGroup = {};
		VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;

		void Destroy();
	};

	struct VulkanShaderCold
	{
		VkPipeline Find(ShaderDescriptor::RenderPipeline::PackedVariant key, uint32_t* pipelineIndex);
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
			Span<const BitFlags<ShaderStage>> specializationConstantStages;
		};

		VkPipeline GetOrCreatePipeline(const PipelineConfig& config, bool forceCreateNewAndRemoveOld = false);
		VkPipeline CreatePipeline(const PipelineConfig& config);
		VkPipeline CreateComputePipeline(const PipelineConfig& config);

		struct SpecializationData
		{
			StaticDArray<VkSpecializationMapEntry, 8> entries;
			StaticDArray<uint8_t, 64> data;
			VkSpecializationInfo info{};
		};

		void BuildSpecializationInfo(ShaderStage stage, SpecializationData& specializationData, const PipelineConfig& config);

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

		static constexpr uint32_t MaxVariants = 16;
		static constexpr uint32_t MaxSpecializationConstants = 8;

		alignas(64) std::array<VariantEntry, MaxVariants> m_Entries;
		std::array<BitFlags<ShaderStage>, MaxSpecializationConstants> m_SpecializationConstantStages;

		StaticDArray<Handle<BindGroupLayout>, 4> m_ReflectedBindGroupLayouts;

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
