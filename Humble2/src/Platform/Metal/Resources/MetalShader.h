#pragma once

#include "Base.h"
#include "Resources/TypeDescriptors.h"

#include "Platform/Metal/MetalDevice.h"
#include "Platform/Metal/MetalRenderer.h"

#include "Platform/Metal/MetalCommon.h"

#include "Platform/Metal/Resources/MetalRenderPass.h"

namespace HBL2
{
    struct MetalShaderHot
    {
        void* Pso = nullptr;
        MTL::DepthStencilState* DepthStencilState = nullptr;
        Handle<BindGroup> ShaderBindGroup = {};
        
        StaticArray<uint16_t, 4> BuffersInBindGroups;
        StaticArray<uint16_t, 4> TexturesInBindGroups;
        
        void Destroy();
    };

    struct MetalShaderCold
    {
        void* Find(ShaderDescriptor::RenderPipeline::PackedVariant key, uint32_t* pipelineIndex);
        void Destroy();
        void DestroyOldShaderModules();
        
        const char* DebugName = "";
        
        MTL4::LibraryFunctionDescriptor* VertexShaderModule = nullptr;
        MTL4::LibraryFunctionDescriptor* FragmentShaderModule = nullptr;
        MTL4::LibraryFunctionDescriptor* ComputeShaderModule = nullptr;
        
        MTL4::LibraryFunctionDescriptor* OldVertexShaderModule = nullptr;
        MTL4::LibraryFunctionDescriptor* OldFragmentShaderModule = nullptr;
        MTL4::LibraryFunctionDescriptor* OldComputeShaderModule = nullptr;
        
        ShaderDescriptor::RenderPipeline::VertexBufferBinding VertexBufferBinding;
        
        MTL4::Compiler* Compiler = nullptr;
        
        struct PipelineConfig
        {
            std::array<MTL4::LibraryFunctionDescriptor*, 2> shaderModules;
            std::array<const char*, 2> entryPoints;
            uint32_t shaderModuleCount = 2;
            ShaderDescriptor::RenderPipeline::PackedVariant variantDesc{};
            Span<const ShaderDescriptor::RenderPipeline::VertexBufferBinding> vertexBufferBindings;
            Span<const BitFlags<ShaderStage>> specializationConstantStages;
            MetalShaderHot* shaderHotData = nullptr;
        };

        uint64_t GetOrCreatePipeline(const PipelineConfig& config, bool forceCreateNewAndRemoveOld = false);
        MTL::RenderPipelineState* CreatePipeline(const PipelineConfig& config);
        MTL::ComputePipelineState* CreateComputePipeline(const PipelineConfig& config);
        
        std::atomic<uint32_t> m_Count{ 0 };
        mutable std::mutex m_WriteMutex;
        
        struct VariantEntry
        {
            ShaderDescriptor::RenderPipeline::PackedVariant Key = g_NullVariant;
            void* Pipeline = nullptr;
            MTL::DepthStencilState* DepthStencilState = nullptr;
        };

        static constexpr uint32_t MaxVariants = 16;
        static constexpr uint32_t MaxSpecializationConstants = 8;

        std::array<VariantEntry, MaxVariants> m_Entries;
        
        uint32_t ColorAttachmentCount = 0;
        StaticDArray<MTL::PixelFormat, 4> ColorAttachmentFormats;
    };

    struct MetalShader
    {
        MetalShader() = default;

        void Initialize(const ShaderDescriptor&& desc);
        void Recompile(const ShaderDescriptor&& desc, bool removeVariants = false);
        void Destroy();
        
        uint64_t GetOrCreateVariant(ShaderDescriptor::RenderPipeline::PackedVariant key);
        uint64_t GetOrCreateComputeVariant(ShaderDescriptor::RenderPipeline::PackedVariant key);
        
        bool IsValid() const;
        
        MetalShaderHot* Hot = nullptr;
        MetalShaderCold* Cold = nullptr;
    };
}
