#include "MetalResourceManager.h"

namespace HBL2
{
    void MetalResourceManager::Initialize(const ResourceManagerSpecification& spec)
    {
        m_Spec = spec;

        InternalInitialize();

//        m_TexturePool.Initialize(m_Spec.Textures);
//        m_BufferSplitPool.Initialize(m_Spec.Buffers);
//        m_ShaderSplitPool.Initialize(m_Spec.Shaders);
//        m_FrameBufferPool.Initialize(m_Spec.FrameBuffers);
//        m_BindGroupSplitPool.Initialize(m_Spec.BindGroups);
//        m_BindGroupLayoutPool.Initialize(m_Spec.BindGroupLayouts);
//        m_RenderPassPool.Initialize(m_Spec.RenderPass);
//        m_RenderPassLayoutPool.Initialize(m_Spec.RenderPassLayouts);
    }
    const ResourceManagerSpecification MetalResourceManager::GetUsageStats()
    {
        ResourceManagerSpecification currentSpec =
        {
//            .Textures = m_TexturePool.FreeSlotCount(),
//            .Buffers = m_BufferSplitPool.FreeSlotCount(),
//            .FrameBuffers = m_FrameBufferPool.FreeSlotCount(),
//            .Shaders = m_ShaderSplitPool.FreeSlotCount(),
//            .BindGroups = m_BindGroupSplitPool.FreeSlotCount(),
//            .BindGroupLayouts = m_BindGroupLayoutPool.FreeSlotCount(),
//            .RenderPass = m_RenderPassPool.FreeSlotCount(),
//            .RenderPassLayouts = m_RenderPassLayoutPool.FreeSlotCount(),
            .Meshes = m_MeshPool.FreeSlotCount(),
            .Materials = m_MaterialPool.FreeSlotCount(),
            .Scenes = m_ScenePool.FreeSlotCount(),
            .Scripts = m_ScriptPool.FreeSlotCount(),
            .Sounds = m_SoundPool.FreeSlotCount(),
            .Prefabs = m_PrefabPool.FreeSlotCount(),
        };

        return currentSpec;
    }
    void MetalResourceManager::Clean()
    {
    }

    // Textures
    Handle<Texture> MetalResourceManager::CreateTexture(const TextureDescriptor&& desc)
    {
//        return m_TexturePool.Insert(std::forward<const TextureDescriptor>(desc));
        return {};
    }
    void MetalResourceManager::DeleteTexture(Handle<Texture> handle)
    {
//        m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=, this]()
//        {
//            MetalTexture* texture = GetTexture(handle);
//            if (texture != nullptr)
//            {
//                texture->Destroy();
//                m_TexturePool.Remove(handle);
//            }
//        });
    }
    void MetalResourceManager::UpdateTexture(Handle<Texture> handle, const Span<const std::byte>& bytes)
    {
//        MetalTexture* texture = GetTexture(handle);
//        if (texture != nullptr)
//        {
//            texture->Update(bytes);
//        }
    }
    void MetalResourceManager::ChangeTextureView(Handle<Texture> handle, const TextureViewDescriptor&& desc)
    {
//        MetalTexture* texture = GetTexture(handle);
//        if (texture != nullptr)
//        {
//            texture->ChangeTextureView(std::forward<const TextureViewDescriptor>(desc));
//        }
    }
    void MetalResourceManager::TransitionTextureLayout(CommandBuffer* commandBuffer, Handle<Texture> handle, TextureLayout currentLayout, TextureLayout newLayout)
    {
//        MetalTexture* texture = GetTexture(handle);
//        if (texture != nullptr)
//        {
//            texture->TrasitionLayout((MetalCommandBuffer*)commandBuffer, currentLayout, newLayout);
//        }
    }
    glm::vec3 MetalResourceManager::GetTextureDimensions(Handle<Texture> handle)
    {
//        MetalTexture* texture = GetTexture(handle);
//        if (texture != nullptr)
//        {
//            return { texture->Extent.width, texture->Extent.height, texture->Extent.depth };
//        }

        return { 0.f, 0.f, 0.f };
    }
    void* MetalResourceManager::GetTextureData(Handle<Texture> handle)
    {
        return nullptr;
    }
//    MetalTexture* MetalResourceManager::GetTexture(Handle<Texture> handle) const
//    {
//        return m_TexturePool.Get(handle);
//    }

    // Buffers
    Handle<Buffer> MetalResourceManager::CreateBuffer(const BufferDescriptor&& desc)
    {
//        MetalBuffer buffer;
//        Handle<Buffer> bufferHandle = m_BufferSplitPool.Insert(&buffer.Hot, &buffer.Cold);
//        buffer.Initialize(std::forward<const BufferDescriptor>(desc));
//        return bufferHandle;
        return {};
    }
    void MetalResourceManager::DeleteBuffer(Handle<Buffer> handle)
    {
//        m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=, this]()
//        {
//            MetalBuffer buffer = GetBuffer(handle);
//            if (buffer.IsValid())
//            {
//                buffer.Destroy();
//                m_BufferSplitPool.Remove(handle);
//            }
//        });
    }
    void MetalResourceManager::ReAllocateBuffer(Handle<Buffer> handle, uint32_t currentOffset)
    {
//        MetalBuffer buffer = GetBuffer(handle);
//        buffer.ReAllocate(currentOffset);
    }
    void* MetalResourceManager::GetBufferData(Handle<Buffer> handle)
    {
//        return GetBufferHot(handle)->Data;
        return nullptr;
    }
    void MetalResourceManager::SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData)
    {
//        MetalBufferHot* MetalBuffer = GetBufferHot(buffer);
//        MetalBuffer->Data = (void*)((char*)newData + offset);
    }
    void MetalResourceManager::SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData)
    {
//        MetalBindGroupCold* MetalBindGroupCold = GetBindGroupCold(bindGroup);
//        if (bufferIndex < MetalBindGroupCold->Buffers.size())
//        {
//            SetBufferData(MetalBindGroupCold->Buffers[bufferIndex].buffer, MetalBindGroupCold->Buffers[bufferIndex].byteOffset, newData);
//        }
    }
    void MetalResourceManager::MapBufferData(Handle<Buffer> buffer, intptr_t offset, intptr_t size)
    {
//        MetalRenderer* renderer = (MetalRenderer*)Renderer::Instance;
//
//        MetalBufferHot* MetalBuffer = GetBufferHot(buffer);
//
//        if (MetalBuffer == nullptr)
//        {
//            return;
//        }
//
//        void* data;
//        vmaMapMemory(renderer->GetAllocator(), MetalBuffer->Allocation, &data);
//        memcpy((void*)((char*)data + offset), (void*)((char*)MetalBuffer->Data + offset), size);
//        vmaUnmapMemory(renderer->GetAllocator(), MetalBuffer->Allocation);
    }
    void MetalResourceManager::MapBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, intptr_t offset, intptr_t size)
    {
//        MetalBindGroupCold* MetalBindGroupCold = GetBindGroupCold(bindGroup);
//        if (MetalBindGroupCold != nullptr && bufferIndex < MetalBindGroupCold->Buffers.size())
//        {
//            if (size != 0)
//            {
//                MapBufferData(MetalBindGroupCold->Buffers[bufferIndex].buffer, offset, size);
//                return;
//            }
//
//            MetalBufferHot* MetalBuffer = GetBufferHot(MetalBindGroupCold->Buffers[bufferIndex].buffer);
//
//            if (MetalBuffer != nullptr)
//            {
//                MapBufferData(MetalBindGroupCold->Buffers[bufferIndex].buffer, offset, MetalBuffer->ByteSize);
//            }
//        }
    }
//    MetalBuffer MetalResourceManager::GetBuffer(Handle<Buffer> handle) const
//    {
//        MetalBuffer buffer;
//        if (m_BufferSplitPool.Get(handle, &buffer.Hot, &buffer.Cold))
//        {
//            return buffer;
//        }
//
//        return {};
//    }
//    MetalBufferHot* MetalResourceManager::GetBufferHot(Handle<Buffer> handle) const
//    {
//        return m_BufferSplitPool.GetHot(handle);
//    }
//    MetalBufferCold* MetalResourceManager::GetBufferCold(Handle<Buffer> handle) const
//    {
//        return m_BufferSplitPool.GetCold(handle);
//    }

    // Framebuffers
    Handle<FrameBuffer> MetalResourceManager::CreateFrameBuffer(const FrameBufferDescriptor&& desc)
    {
//        return m_FrameBufferPool.Insert(std::forward<const FrameBufferDescriptor>(desc));
        return {};
    }
    void MetalResourceManager::DeleteFrameBuffer(Handle<FrameBuffer> handle)
    {
//        m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=, this]()
//        {
//            MetalFrameBuffer* frameBuffer = GetFrameBuffer(handle);
//            if (frameBuffer != nullptr)
//            {
//                frameBuffer->Destroy();
//                m_FrameBufferPool.Remove(handle);
//            }
//        });
    }
    void MetalResourceManager::ResizeFrameBuffer(Handle<FrameBuffer> handle, uint32_t width, uint32_t height)
    {
//        if (!handle.IsValid())
//        {
//            return;
//        }
//
//        MetalFrameBuffer* frameBuffer = GetFrameBuffer(handle);
//        frameBuffer->Resize(width, height);
    }
//    MetalFrameBuffer* MetalResourceManager::GetFrameBuffer(Handle<FrameBuffer> handle) const
//    {
//        return m_FrameBufferPool.Get(handle);
//    }

    // Shaders
    Handle<Shader> MetalResourceManager::CreateShader(const ShaderDescriptor&& desc)
    {
//        MetalShader shader;
//        Handle<Shader> shaderHandle = m_ShaderSplitPool.Insert(&shader.Hot, &shader.Cold);
//        shader.Initialize(std::forward<const ShaderDescriptor>(desc));
//        return shaderHandle;
        return {};
    }
    void MetalResourceManager::RecompileShader(Handle<Shader> handle, const ShaderDescriptor&& desc)
    {
//        MetalShader shader = GetShader(handle);
//        shader.Recompile(std::forward<const ShaderDescriptor>(desc), true);
//
//        m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=, this]()
//        {
//            MetalShader shader = GetShader(handle);
//            shader.DestroyOld();
//        });
    }
    void MetalResourceManager::DeleteShader(Handle<Shader> handle)
    {
//        m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=, this]()
//        {
//            MetalShader shader = GetShader(handle);
//            if (shader.IsValid())
//            {
//                shader.Destroy();
//                m_ShaderSplitPool.Remove(handle);
//            }
//        });
    }
    uint64_t MetalResourceManager::GetOrAddShaderVariant(Handle<Shader> handle, const ShaderDescriptor::RenderPipeline::PackedVariant& variantDesc)
    {
//        MetalShader shader = GetShader(handle);
//        return (uint64_t)shader.GetOrCreateVariant(variantDesc);
        return 0;
    }
    void MetalResourceManager::SetShaderGlobalBindGroup(Handle<Shader> handle, Handle<BindGroup> bindGroupHandle)
    {
//        MetalShaderHot* shader = GetShaderHot(handle);
//
//        if (shader != nullptr)
//        {
//            shader->ShaderBindGroup = bindGroupHandle;
//        }
    }
    Handle<BindGroup> MetalResourceManager::GetShaderGlobalBindGroup(Handle<Shader> handle)
    {
//        MetalShaderHot* shader = GetShaderHot(handle);
//
//        if (shader != nullptr)
//        {
//            return shader->ShaderBindGroup;
//        }
//
//        return Renderer::Instance->GetEmptyBindings();
        return {};
    }
//    MetalShader MetalResourceManager::GetShader(Handle<Shader> handle) const
//    {
//        MetalShader shader;
//        if (m_ShaderSplitPool.Get(handle, &shader.Hot, &shader.Cold))
//        {
//            return shader;
//        }
//
//        return {};
//    }
//    MetalShaderHot* MetalResourceManager::GetShaderHot(Handle<Shader> handle) const
//    {
//        return m_ShaderSplitPool.GetHot(handle);
//    }
//    MetalShaderCold* MetalResourceManager::GetShaderCold(Handle<Shader> handle) const
//    {
//        return m_ShaderSplitPool.GetCold(handle);
//    }

    // BindGroups
    Handle<BindGroup> MetalResourceManager::CreateBindGroup(const BindGroupDescriptor&& desc)
    {
//        // Caching mechanism so that materials with the same resources, use the same bind group.
//        uint16_t index = 0;
//        uint64_t descriptorHash = ResourceManager::Instance->GetBindGroupHash(std::move(desc));
//
//        for (const auto& bindGroup : m_BindGroupSplitPool.GetDataColdPool())
//        {
//            uint64_t hash = CalculateBindGroupHash(&bindGroup);
//
//            if (descriptorHash == hash && bindGroup.DebugName != nullptr)
//            {
//                MetalBindGroupCold* mutBindGroup = (MetalBindGroupCold*)&bindGroup;
//                if (mutBindGroup->TryAddRef())
//                {
//                    return m_BindGroupSplitPool.GetHandleFromIndex(index);
//                }
//
//                break;
//            }
//
//            index++;
//        }
//
//        MetalBindGroup bindgroup;
//        Handle<BindGroup> bg = m_BindGroupSplitPool.Insert(&bindgroup.Hot, &bindgroup.Cold);
//        bindgroup.Initialize(std::move(desc));
//
//        // Increase ref count of bindgroup.
//        bindgroup.Cold->TryAddRef();
//
//        return bg;
        return {};
    }
    void MetalResourceManager::DeleteBindGroup(Handle<BindGroup> handle)
    {
//        MetalBindGroupCold* bindGroupCold = GetBindGroupCold(handle);
//
//        if (bindGroupCold == nullptr)
//        {
//            // Invalid handle or already deleted.
//            return;
//        }
//
//        MetalBindGroupLayout* bindGroupLayout = GetBindGroupLayout(bindGroupCold->BindGroupLayout);
//        if (bindGroupLayout != nullptr && bindGroupLayout->CreatedFromReflection)
//        {
//            DeleteBindGroupLayout(bindGroupCold->BindGroupLayout);
//        }
//
//        if (bindGroupCold->ReleaseRefAndMaybeDelete())
//        {
//            m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=, this]()
//            {
//                MetalBindGroupCold* bindGroupCold = GetBindGroupCold(handle);
//                if (bindGroupCold != nullptr)
//                {
//                    bindGroupCold->Destroy();
//                    m_BindGroupSplitPool.Remove(handle);
//                }
//            });
//        }
    }
    void MetalResourceManager::UpdateBindGroup(Handle<BindGroup> handle)
    {
//        MetalBindGroup bindGroup = GetBindGroup(handle);
//        bindGroup.Update();
    }
    uint64_t MetalResourceManager::GetBindGroupHash(Handle<BindGroup> handle)
    {
//        return CalculateBindGroupHash(GetBindGroupCold(handle));
        return 0;
    }
//    MetalBindGroup MetalResourceManager::GetBindGroup(Handle<BindGroup> handle) const
//    {
//        MetalBindGroup bindGroup;
//        if (m_BindGroupSplitPool.Get(handle, &bindGroup.Hot, &bindGroup.Cold))
//        {
//            return bindGroup;
//        }
//
//        return {};
//    }
//    MetalBindGroupHot* MetalResourceManager::GetBindGroupHot(Handle<BindGroup> handle) const
//    {
//        return m_BindGroupSplitPool.GetHot(handle);
//    }
//    MetalBindGroupCold* MetalResourceManager::GetBindGroupCold(Handle<BindGroup> handle) const
//    {
//        return m_BindGroupSplitPool.GetCold(handle);
//    }
//    uint64_t MetalResourceManager::CalculateBindGroupHash(const MetalBindGroupCold* bindGroupCold)
//    {
//        if (bindGroupCold == nullptr)
//        {
//            return 0;
//        }
//
//        uint64_t hash = 0;
//
//        uint64_t bufferHash = 0x517cc1b727220a95ULL;
//        uint64_t textureHash = 0x9e3779b97f4a7c15ULL;
//
//        for (const auto& bufferEntry : bindGroupCold->Buffers)
//        {
//            HashCombine(bufferHash, bufferEntry.buffer.HashKey() + typeid(Buffer).hash_code());
//            HashCombine(bufferHash, bufferEntry.byteOffset);
//            HashCombine(bufferHash, bufferEntry.range);
//        }
//
//        for (const auto& textureEntry : bindGroupCold->Textures)
//        {
//            HashCombine(textureHash, textureEntry.texture.HashKey() + typeid(Texture).hash_code());
//            HashCombine(textureHash, static_cast<uint64_t>(textureEntry.desiredLayout));
//        }
//
//        HashCombine(hash, bufferHash);
//        HashCombine(hash, textureHash);
//        HashCombine(hash, bindGroupCold->BindGroupLayout.HashKey());
//
//        return hash;
//    }

    // BindGroupsLayouts
    Handle<BindGroupLayout> MetalResourceManager::CreateBindGroupLayout(const BindGroupLayoutDescriptor&& desc)
    {
//        // Caching mechanism so that bind groups and shaders with the same layout, use the same bind group layout object.
//        uint16_t index = 0;
//        uint64_t layoutHash = ResourceManager::Instance->GetBindGroupLayoutHash(std::move(desc));
//
//        for (const auto& bindGroupLayout : m_BindGroupLayoutPool.GetDataPool())
//        {
//            uint64_t hash = CalculateBindGroupLayoutHash(&bindGroupLayout);
//
//            if (layoutHash == hash && bindGroupLayout.DebugName != nullptr)
//            {
//                MetalBindGroupLayout* mutBindGroupLayout = (MetalBindGroupLayout*)&bindGroupLayout;
//                if (mutBindGroupLayout->TryAddRef())
//                {
//                    return m_BindGroupLayoutPool.GetHandleFromIndex(index);
//                }
//
//                break;
//            }
//
//            index++;
//        }
//
//        Handle<BindGroupLayout> layoutHandle = m_BindGroupLayoutPool.Insert(std::move(desc));
//        MetalBindGroupLayout* bindGroupLayout = GetBindGroupLayout(layoutHandle);
//
//        // Increase ref count of bindgroup layout.
//        bindGroupLayout->TryAddRef();
//
//        return layoutHandle;
        return {};
    }
    void MetalResourceManager::DeleteBindGroupLayout(Handle<BindGroupLayout> handle)
    {
//        MetalBindGroupLayout* bindGroupLayout = GetBindGroupLayout(handle);
//
//        if (bindGroupLayout == nullptr)
//        {
//            // Invalid handle or already deleted.
//            return;
//        }
//
//        if (bindGroupLayout->ReleaseRefAndMaybeDelete())
//        {
//            m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=, this]()
//            {
//                MetalBindGroupLayout* bindGroupLayout = GetBindGroupLayout(handle);
//                if (bindGroupLayout != nullptr)
//                {
//                    bindGroupLayout->Destroy();
//                    m_BindGroupLayoutPool.Remove(handle);
//                }
//            });
//        }
    }
    uint64_t MetalResourceManager::GetBindGroupLayoutHash(Handle<BindGroupLayout> handle)
    {
//        return CalculateBindGroupLayoutHash(GetBindGroupLayout(handle));
        return 0;
    }
//    MetalBindGroupLayout* MetalResourceManager::GetBindGroupLayout(Handle<BindGroupLayout> handle) const
//    {
//        return m_BindGroupLayoutPool.Get(handle);
//    }
//    uint64_t MetalResourceManager::CalculateBindGroupLayoutHash(const MetalBindGroupLayout* bindGroupLayout)
//    {
//        if (bindGroupLayout == nullptr)
//        {
//            return 0;
//        }
//
//        uint64_t hash = 0;
//
//        uint64_t bufferHash = 0x517cc1b727220a95ULL;
//        uint64_t textureHash = 0x9e3779b97f4a7c15ULL;
//
//        for (const auto& bufferEntry : bindGroupLayout->BufferBindings)
//        {
//            HashCombine(bufferHash, bufferEntry.slot);
//            HashCombine(bufferHash, static_cast<uint64_t>(bufferEntry.type));
//            HashCombine(bufferHash, static_cast<uint64_t>(bufferEntry.visibility));
//        }
//
//        for (const auto& texture : bindGroupLayout->TextureBindings)
//        {
//            HashCombine(textureHash, texture.slot);
//            HashCombine(textureHash, static_cast<uint64_t>(texture.type));
//            HashCombine(textureHash, static_cast<uint64_t>(texture.visibility));
//        }
//
//        HashCombine(hash, bufferHash);
//        HashCombine(hash, textureHash);
//
//        return hash;
//    }
    
    // RenderPass
    Handle<RenderPass> MetalResourceManager::CreateRenderPass(const RenderPassDescriptor&& desc)
    {
//        return m_RenderPassPool.Insert(std::forward<const RenderPassDescriptor>(desc));
        return {};
    }
    void MetalResourceManager::DeleteRenderPass(Handle<RenderPass> handle)
    {
//        m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=, this]()
//        {
//            MetalRenderPass* renderPass = GetRenderPass(handle);
//            if (renderPass != nullptr)
//            {
//                renderPass->Destroy();
//                m_RenderPassPool.Remove(handle);
//            }
//        });
    }
//    MetalRenderPass* MetalResourceManager::GetRenderPass(Handle<RenderPass> handle) const
//    {
//        return m_RenderPassPool.Get(handle);
//    }

    // RenderPassLayouts
    Handle<RenderPassLayout> MetalResourceManager::CreateRenderPassLayout(const RenderPassLayoutDescriptor&& desc)
    {
//        return m_RenderPassLayoutPool.Insert(std::forward<const RenderPassLayoutDescriptor>(desc));
        return {};
    }
    void MetalResourceManager::DeleteRenderPassLayout(Handle<RenderPassLayout> handle)
    {
    }
//    MetalRenderPassLayout* MetalResourceManager::GetRenderPassLayout(Handle<RenderPassLayout> handle) const
//    {
//        return m_RenderPassLayoutPool.Get(handle);
//    }
}


