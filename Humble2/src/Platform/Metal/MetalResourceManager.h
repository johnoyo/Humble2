#pragma once

#include "Resources/ResourceManager.h"

#include "Resources/Pool.h"
#include "Resources/SplitPool.h"
#include "Resources/Types.h"
#include "Resources/TypeDescriptors.h"

#include "Platform/Metal/Resources/MetalBuffer.h"
#include "Platform/Metal/Resources/MetalShader.h"
#include "Platform/Metal/Resources/MetalTexture.h"
#include "Platform/Metal/Resources/MetalBindGroup.h"
#include "Platform/Metal/Resources/MetalBindGroupLayout.h"
#include "Platform/Metal/Resources/MetalRenderPass.h"
#include "Platform/Metal/Resources/MetalRenderPassLayout.h"

namespace HBL2
{
    class MetalResourceManager final : public ResourceManager
    {
    public:
        virtual ~MetalResourceManager() = default;

        virtual void Initialize(const ResourceManagerSpecification& spec) override;
        virtual const ResourceManagerSpecification GetUsageStats() override;
        virtual void Clean() override;

        // Textures
        virtual Handle<Texture> CreateTexture(const TextureDescriptor&& desc) override;
        virtual void DeleteTexture(Handle<Texture> handle) override;
        virtual void UpdateTexture(Handle<Texture> handle, const Span<const std::byte>& bytes) override;
        virtual void ChangeTextureView(Handle<Texture> handle, const TextureViewDescriptor&& desc) override;
        virtual void TransitionTextureLayout(CommandBuffer* commandBuffer, Handle<Texture> handle, TextureLayout currentLayout, TextureLayout newLayout) override;
        virtual glm::vec3 GetTextureDimensions(Handle<Texture> handle) override;
        virtual void* GetTextureData(Handle<Texture> handle) override;
        MetalTexture* GetTexture(Handle<Texture> handle) const;

        // Buffers
        virtual Handle<Buffer> CreateBuffer(const BufferDescriptor&& desc) override;
        virtual void DeleteBuffer(Handle<Buffer> handle) override;
        virtual void ReAllocateBuffer(Handle<Buffer> handle, uint32_t currentOffset) override;
        virtual void* GetBufferData(Handle<Buffer> handle) override;
        virtual void SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData) override;
        virtual void SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData) override;
        virtual void MapBufferData(Handle<Buffer> buffer, intptr_t offset, intptr_t size) override;
        virtual void MapBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, intptr_t offset = 0, intptr_t size = 0) override;
        //MetalBuffer GetBuffer(Handle<Buffer> handle) const;
        //MetalBufferHot* GetBufferHot(Handle<Buffer> handle) const;
        //MetalBufferCold* GetBufferCold(Handle<Buffer> handle) const;

        // Shaders
        virtual Handle<Shader> CreateShader(const ShaderDescriptor&& desc) override;
        virtual void RecompileShader(Handle<Shader> handle, const ShaderDescriptor&& desc) override;
        virtual void DeleteShader(Handle<Shader> handle) override;
        virtual uint64_t GetOrAddShaderVariant(Handle<Shader> handle, const ShaderDescriptor::RenderPipeline::PackedVariant& variantDesc) override;
        virtual void SetShaderGlobalBindGroup(Handle<Shader> handle, Handle<BindGroup> bindGroupHandle) override;
        virtual Handle<BindGroup> GetShaderGlobalBindGroup(Handle<Shader> handle) override;
        //MetalShader GetShader(Handle<Shader> handle) const;
        //MetalShaderHot* GetShaderHot(Handle<Shader> handle) const;
        //MetalShaderCold* GetShaderCold(Handle<Shader> handle) const;

        // BindGroups
        virtual Handle<BindGroup> CreateBindGroup(const BindGroupDescriptor&& desc) override;
        virtual void DeleteBindGroup(Handle<BindGroup> handle) override;
        virtual void UpdateBindGroup(Handle<BindGroup> handle) override;
        virtual uint64_t GetBindGroupHash(Handle<BindGroup> handle) override;
        //MetalBindGroup GetBindGroup(Handle<BindGroup> handle) const;
        //MetalBindGroupHot* GetBindGroupHot(Handle<BindGroup> handle) const;
        //MetalBindGroupCold* GetBindGroupCold(Handle<BindGroup> handle) const;

        // BindGroupsLayouts
        virtual Handle<BindGroupLayout> CreateBindGroupLayout(const BindGroupLayoutDescriptor&& desc) override;
        virtual void DeleteBindGroupLayout(Handle<BindGroupLayout> handle) override;
        virtual uint64_t GetBindGroupLayoutHash(Handle<BindGroupLayout> handle) override;
//        MetalBindGroupLayout* GetBindGroupLayout(Handle<BindGroupLayout> handle) const;

        // RenderPass
        virtual Handle<RenderPass> CreateRenderPass(const RenderPassDescriptor&& desc) override;
        virtual void DeleteRenderPass(Handle<RenderPass> handle) override;
        virtual void RecreateRenderPassFrameBuffer(Handle<RenderPass> handle, const FrameBufferDescriptor&& desc) override;
        MetalRenderPass* GetRenderPass(Handle<RenderPass> handle) const;

        // RenderPassLayouts
        virtual Handle<RenderPassLayout> CreateRenderPassLayout(const RenderPassLayoutDescriptor&& desc) override;
        virtual void DeleteRenderPassLayout(Handle<RenderPassLayout> handle) override;
//        MetalRenderPassLayout* GetRenderPassLayout(Handle<RenderPassLayout> handle) const;

    private:
//        Pool<MetalTexture, Texture> m_TexturePool;
//        SplitPool<MetalBufferHot, MetalBufferCold, Buffer> m_BufferSplitPool;
//        SplitPool<MetalShaderHot, MetalShaderCold, Shader> m_ShaderSplitPool;
//        SplitPool<MetalBindGroupHot, MetalBindGroupCold, BindGroup> m_BindGroupSplitPool;
//        Pool<MetalBindGroupLayout, BindGroupLayout> m_BindGroupLayoutPool;
//        Pool<MetalRenderPass, RenderPass> m_RenderPassPool;
//        Pool<MetalRenderPassLayout, RenderPassLayout> m_RenderPassLayoutPool;

        friend class VulkanRenderer; // This is required for a hack to create the swapchain images in the VulkanRenderer

    private:
        // uint64_t CalculateBindGroupHash(const MetalBindGroupCold* bindGroupCold);
        // uint64_t CalculateBindGroupLayoutHash(const MetalBindGroupLayout* bindGroupLayout);
    };
}

