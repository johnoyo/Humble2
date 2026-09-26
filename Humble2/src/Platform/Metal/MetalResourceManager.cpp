#include "MetalResourceManager.h"

#include "Renderer/Renderer.h"

namespace HBL2
{
    void MetalResourceManager::Initialize(const ResourceManagerSpecification& spec)
    {
        m_Spec = spec;

        InternalInitialize();

        m_TexturePool.Initialize(m_Spec.Textures);
        m_BufferSplitPool.Initialize(m_Spec.Buffers);
        m_ShaderSplitPool.Initialize(m_Spec.Shaders);
        m_BindGroupSplitPool.Initialize(m_Spec.BindGroups);
        m_BindGroupLayoutPool.Initialize(m_Spec.BindGroupLayouts);
        m_RenderPassPool.Initialize(m_Spec.RenderPass);
        m_RenderPassLayoutPool.Initialize(m_Spec.RenderPassLayouts);
    }
    const ResourceManagerSpecification MetalResourceManager::GetUsageStats()
    {
        ResourceManagerSpecification currentSpec =
        {
            .Textures = m_TexturePool.FreeSlotCount(),
            .Buffers = m_BufferSplitPool.FreeSlotCount(),
            .Shaders = m_ShaderSplitPool.FreeSlotCount(),
            .BindGroups = m_BindGroupSplitPool.FreeSlotCount(),
            .BindGroupLayouts = m_BindGroupLayoutPool.FreeSlotCount(),
            .RenderPass = m_RenderPassPool.FreeSlotCount(),
            .RenderPassLayouts = m_RenderPassLayoutPool.FreeSlotCount(),
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
        return m_TexturePool.Insert(std::forward<const TextureDescriptor>(desc));
    }
    void MetalResourceManager::ReimportTexture(Handle<Texture> handle, const TextureDescriptor&& desc)
    {

    }
    void MetalResourceManager::DeleteTexture(Handle<Texture> handle)
    {
        m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=, this]()
        {
            MetalTexture* texture = GetTexture(handle);
            if (texture != nullptr)
            {
                texture->Destroy();
                m_TexturePool.Remove(handle);
            }
        });
    }
    void MetalResourceManager::UpdateTexture(Handle<Texture> handle, const Span<const std::byte>& bytes)
    {
        MetalTexture* texture = GetTexture(handle);
        if (texture != nullptr)
        {
            texture->Update(bytes);
        }
    }
    void MetalResourceManager::ChangeTextureView(Handle<Texture> handle, const TextureViewDescriptor&& desc)
    {
        MetalTexture* texture = GetTexture(handle);
        if (texture != nullptr)
        {
            texture->ChangeTextureView(std::forward<const TextureViewDescriptor>(desc));
        }
    }
    void MetalResourceManager::TransitionTextureLayout(CommandBuffer* commandBuffer, Handle<Texture> handle, TextureLayout currentLayout, TextureLayout newLayout)
    {
        MetalTexture* texture = GetTexture(handle);
        if (texture != nullptr)
        {
            texture->SynchronizeUsage((MetalCommandBuffer*)commandBuffer, currentLayout, newLayout);
        }
    }
    glm::vec3 MetalResourceManager::GetTextureDimensions(Handle<Texture> handle)
    {
        MetalTexture* texture = GetTexture(handle);
        if (texture != nullptr)
        {
            return { texture->Extent.width, texture->Extent.height, texture->Extent.depth };
        }

        return { 0.f, 0.f, 0.f };
    }
    void* MetalResourceManager::GetTextureData(Handle<Texture> handle)
    {
        return nullptr;
    }
    MetalTexture* MetalResourceManager::GetTexture(Handle<Texture> handle) const
    {
        return m_TexturePool.Get(handle);
        return nullptr;
    }

    // Buffers
    Handle<Buffer> MetalResourceManager::CreateBuffer(const BufferDescriptor&& desc)
    {
        MetalBuffer buffer;
        Handle<Buffer> bufferHandle = m_BufferSplitPool.Insert(&buffer.Hot, &buffer.Cold);
        buffer.Initialize(std::forward<const BufferDescriptor>(desc));
        return bufferHandle;
    }
    void MetalResourceManager::DeleteBuffer(Handle<Buffer> handle)
    {
        m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=, this]()
        {
            MetalBuffer buffer = GetBuffer(handle);
            if (buffer.IsValid())
            {
                buffer.Destroy();
                m_BufferSplitPool.Remove(handle);
            }
        });
    }
    void MetalResourceManager::ReAllocateBuffer(Handle<Buffer> handle, uint32_t currentOffset)
    {
        // deprecated to be removed.
    }
    void* MetalResourceManager::GetBufferData(Handle<Buffer> handle)
    {
        return GetBufferHot(handle)->Data;
    }
    void MetalResourceManager::SetBufferData(Handle<Buffer> buffer, intptr_t offset, void* newData)
    {
        MetalBufferHot* MetalBuffer = GetBufferHot(buffer);
        MetalBuffer->Data = (void*)((char*)newData + offset);
    }
    void MetalResourceManager::SetBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, void* newData)
    {
        MetalBindGroupCold* mtlBindGroupCold = GetBindGroupCold(bindGroup);
        if (bufferIndex < mtlBindGroupCold->Buffers.size())
        {
            SetBufferData(mtlBindGroupCold->Buffers[bufferIndex].buffer, mtlBindGroupCold->Buffers[bufferIndex].byteOffset, newData);
        }
    }
    void MetalResourceManager::MapBufferData(Handle<Buffer> buffer, intptr_t offset, intptr_t size)
    {
        MetalBufferHot* metalBuffer = GetBufferHot(buffer);

        if (metalBuffer == nullptr)
        {
            return;
        }

        memcpy((void*)((char*)metalBuffer->Buffer->contents() + offset), (void*)((char*)metalBuffer->Data + offset), size);
    }
    void MetalResourceManager::MapBufferData(Handle<BindGroup> bindGroup, uint32_t bufferIndex, intptr_t offset, intptr_t size)
    {
        MetalBindGroupCold* mtlBindGroupCold = GetBindGroupCold(bindGroup);
        if (mtlBindGroupCold != nullptr && bufferIndex < mtlBindGroupCold->Buffers.size())
        {
            if (size != 0)
            {
                MapBufferData(mtlBindGroupCold->Buffers[bufferIndex].buffer, offset, size);
                return;
            }

            MetalBufferHot* mtlBuffer = GetBufferHot(mtlBindGroupCold->Buffers[bufferIndex].buffer);

            if (mtlBuffer != nullptr)
            {
                MapBufferData(mtlBindGroupCold->Buffers[bufferIndex].buffer, offset, mtlBuffer->ByteSize);
            }
        }
    }
    MetalBuffer MetalResourceManager::GetBuffer(Handle<Buffer> handle) const
    {
        MetalBuffer buffer;
        if (m_BufferSplitPool.Get(handle, &buffer.Hot, &buffer.Cold))
        {
            return buffer;
        }

        return {};
    }
    MetalBufferHot* MetalResourceManager::GetBufferHot(Handle<Buffer> handle) const
    {
        return m_BufferSplitPool.GetHot(handle);
    }
    MetalBufferCold* MetalResourceManager::GetBufferCold(Handle<Buffer> handle) const
    {
        return m_BufferSplitPool.GetCold(handle);
    }

    // Shaders
    Handle<Shader> MetalResourceManager::CreateShader(const ShaderDescriptor&& desc)
    {
        MetalShader shader;
        Handle<Shader> shaderHandle = m_ShaderSplitPool.Insert(&shader.Hot, &shader.Cold);
        shader.Initialize(std::forward<const ShaderDescriptor>(desc));
        return shaderHandle;
    }
    void MetalResourceManager::RecompileShader(Handle<Shader> handle, const ShaderDescriptor&& desc)
    {
        MetalShader shader = GetShader(handle);
        shader.Recompile(std::forward<const ShaderDescriptor>(desc), true);
    }
    void MetalResourceManager::DeleteShader(Handle<Shader> handle)
    {
        m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=, this]()
        {
            MetalShader shader = GetShader(handle);
            if (shader.IsValid())
            {
                shader.Destroy();
                m_ShaderSplitPool.Remove(handle);
            }
        });
    }
    uint64_t MetalResourceManager::GetOrAddShaderVariant(Handle<Shader> handle, const ShaderDescriptor::RenderPipeline::PackedVariant& variantDesc)
    {
        MetalShader shader = GetShader(handle);
        return shader.GetOrCreateVariant(variantDesc); // return the pipeline packed variant.
    }
    void MetalResourceManager::SetShaderGlobalBindGroup(Handle<Shader> handle, Handle<BindGroup> bindGroupHandle)
    {
        MetalShaderHot* shader = GetShaderHot(handle);

        if (shader != nullptr)
        {
            shader->ShaderBindGroup = bindGroupHandle;
        }
    }
    Handle<BindGroup> MetalResourceManager::GetShaderGlobalBindGroup(Handle<Shader> handle)
    {
        MetalShaderHot* shader = GetShaderHot(handle);

        if (shader != nullptr)
        {
            return shader->ShaderBindGroup.Get();
        }

        return Renderer::Instance->GetEmptyBindings();
    }
    MetalShader MetalResourceManager::GetShader(Handle<Shader> handle) const
    {
        MetalShader shader;
        if (m_ShaderSplitPool.Get(handle, &shader.Hot, &shader.Cold))
        {
            return shader;
        }

        return {};
    }
    MetalShaderHot* MetalResourceManager::GetShaderHot(Handle<Shader> handle) const
    {
        return m_ShaderSplitPool.GetHot(handle);
    }
    MetalShaderCold* MetalResourceManager::GetShaderCold(Handle<Shader> handle) const
    {
        return m_ShaderSplitPool.GetCold(handle);
    }

    // BindGroups
    Handle<BindGroup> MetalResourceManager::CreateBindGroup(const BindGroupDescriptor&& desc)
    {
        // Caching mechanism so that materials with the same resources, use the same bind group.
        uint16_t index = 0;
        uint64_t descriptorHash = ResourceManager::Instance->GetBindGroupHash(desc);

        for (const auto& bindGroup : m_BindGroupSplitPool.GetDataColdPool())
        {
            if (descriptorHash == bindGroup.Hash && bindGroup.DebugName != nullptr)
            {
                Handle<BindGroup> bindGroupHandle = m_BindGroupSplitPool.GetHandleFromIndex(index);

                if (!m_BindGroupSplitPool.IsClosing(bindGroupHandle))
                {
                    return bindGroupHandle;
                }
            }

            index++;
        }

        MetalBindGroup bindgroup;
        Handle<BindGroup> bg = m_BindGroupSplitPool.Insert(&bindgroup.Hot, &bindgroup.Cold);
        bindgroup.Initialize(std::forward<const BindGroupDescriptor>(desc));

        return bg;
    }
    void MetalResourceManager::DeleteBindGroup(Handle<BindGroup> handle)
    {
        if (!m_BindGroupSplitPool.IsAlive(handle))
        {
            m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=, this]()
            {
                MetalBindGroupCold* bindGroupCold = GetBindGroupCold(handle);
                if (bindGroupCold != nullptr)
                {
                    bindGroupCold->Destroy();
                    m_BindGroupSplitPool.Remove(handle);
                }
            });
        }
    }
    void MetalResourceManager::UpdateBindGroup(Handle<BindGroup> handle)
    {
        //MetalBindGroup bindGroup = GetBindGroup(handle);
        //bindGroup.Update();
    }
    uint64_t MetalResourceManager::GetBindGroupHash(Handle<BindGroup> handle)
    {
        MetalBindGroupCold* bindGroupCold = GetBindGroupCold(handle);
        if (bindGroupCold != nullptr)
        {
            return bindGroupCold->Hash;
        }
        
        return 0;
    }
    MetalBindGroup MetalResourceManager::GetBindGroup(Handle<BindGroup> handle) const
    {
        MetalBindGroup bindGroup;
        if (m_BindGroupSplitPool.Get(handle, &bindGroup.Hot, &bindGroup.Cold))
        {
            return bindGroup;
        }

        return {};
    }
    MetalBindGroupHot* MetalResourceManager::GetBindGroupHot(Handle<BindGroup> handle) const
    {
        return m_BindGroupSplitPool.GetHot(handle);
    }
    MetalBindGroupCold* MetalResourceManager::GetBindGroupCold(Handle<BindGroup> handle) const
    {
        return m_BindGroupSplitPool.GetCold(handle);
    }

    // BindGroupsLayouts
    Handle<BindGroupLayout> MetalResourceManager::CreateBindGroupLayout(const BindGroupLayoutDescriptor&& desc)
    {
        // Caching mechanism so that bind groups and shaders with the same layout, use the same bind group layout object.
        uint16_t index = 0;
        uint64_t layoutHash = ResourceManager::Instance->GetBindGroupLayoutHash(desc);

        for (const auto& bindGroupLayout : m_BindGroupLayoutPool.GetDataPool())
        {
            if (layoutHash == bindGroupLayout.Hash && bindGroupLayout.DebugName != nullptr)
            {
                Handle<BindGroupLayout> bindGroupLayoutHandle = m_BindGroupLayoutPool.GetHandleFromIndex(index);

                if (!m_BindGroupLayoutPool.IsClosing(bindGroupLayoutHandle))
                {
                    return bindGroupLayoutHandle;
                }
            }

            index++;
        }

        return m_BindGroupLayoutPool.Insert(std::forward<const BindGroupLayoutDescriptor>(desc));
    }
    void MetalResourceManager::DeleteBindGroupLayout(Handle<BindGroupLayout> handle)
    {
        if (!m_BindGroupLayoutPool.IsAlive(handle))
        {
            m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=, this]()
            {
                MetalBindGroupLayout* bindGroupLayout = GetBindGroupLayout(handle);
                if (bindGroupLayout != nullptr)
                {
                    bindGroupLayout->Destroy();
                    m_BindGroupLayoutPool.Remove(handle);
                }
            });
        }
    }
    uint64_t MetalResourceManager::GetBindGroupLayoutHash(Handle<BindGroupLayout> handle)
    {
        MetalBindGroupLayout* bindGroupLayout = GetBindGroupLayout(handle);
        if (bindGroupLayout != nullptr)
        {
            return bindGroupLayout->Hash;
        }
        
        return 0;
    }
    MetalBindGroupLayout* MetalResourceManager::GetBindGroupLayout(Handle<BindGroupLayout> handle) const
    {
        return m_BindGroupLayoutPool.Get(handle);
    }
    
    // RenderPass
    Handle<RenderPass> MetalResourceManager::CreateRenderPass(const RenderPassDescriptor&& desc)
    {
        return m_RenderPassPool.Insert(std::forward<const RenderPassDescriptor>(desc));
    }
    void MetalResourceManager::DeleteRenderPass(Handle<RenderPass> handle)
    {
        m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=, this]()
        {
            MetalRenderPass* renderPass = GetRenderPass(handle);
            if (renderPass != nullptr)
            {
                renderPass->Destroy();
                m_RenderPassPool.Remove(handle);
            }
        });
    }
    MetalRenderPass* MetalResourceManager::GetRenderPass(Handle<RenderPass> handle) const
    {
        return m_RenderPassPool.Get(handle);
    }

    // RenderPassLayouts
    Handle<RenderPassLayout> MetalResourceManager::CreateRenderPassLayout(const RenderPassLayoutDescriptor&& desc)
    {
        return m_RenderPassLayoutPool.Insert(std::forward<const RenderPassLayoutDescriptor>(desc));
    }
    void MetalResourceManager::DeleteRenderPassLayout(Handle<RenderPassLayout> handle)
    {
        m_DeletionQueue.Push(Renderer::Instance->GetFrameNumber(), [=, this]()
        {
            m_RenderPassLayoutPool.Remove(handle);
        });
    }
    void MetalResourceManager::RecreateRenderPassFrameBuffer(Handle<RenderPass> handle, const FrameBufferDescriptor&& desc)
    {
        MetalRenderPass* rp = GetRenderPass(handle);

        if (rp != nullptr)
        {
            rp->UpdateFrameBuffer(std::forward<const FrameBufferDescriptor>(desc));
        }
    }
    MetalRenderPassLayout* MetalResourceManager::GetRenderPassLayout(Handle<RenderPassLayout> handle) const
    {
        return m_RenderPassLayoutPool.Get(handle);
    }

    void MetalResourceManager::Acquire(uint32_t packedHandle, ResourceType resourceType)
    {
        if (resourceType == ResourceType::BindGroup)
        {
            Handle<BindGroup> handle = Handle<BindGroup>::UnPack(packedHandle);
            m_BindGroupSplitPool.Acquire(handle);
        }
        else if (resourceType == ResourceType::BindGroupLayout)
        {
            Handle<BindGroupLayout> handle = Handle<BindGroupLayout>::UnPack(packedHandle);
            m_BindGroupLayoutPool.Acquire(handle);
        }
        else if (resourceType == ResourceType::Texture)
        {
//            Handle<Texture> handle = Handle<Texture>::UnPack(packedHandle);
//            m_TexturePool.Acquire(handle);
        }
    }
    void MetalResourceManager::Release(uint32_t packedHandle, ResourceType resourceType)
    {
        if (resourceType == ResourceType::BindGroup)
        {
            Handle<BindGroup> handle = Handle<BindGroup>::UnPack(packedHandle);

            if (m_BindGroupSplitPool.Release(handle))
            {
                DeleteBindGroup(handle);
            }
        }
        else if (resourceType == ResourceType::BindGroupLayout)
        {
            Handle<BindGroupLayout> handle = Handle<BindGroupLayout>::UnPack(packedHandle);

            if (m_BindGroupLayoutPool.Release(handle))
            {
                DeleteBindGroupLayout(handle);
            }
        }
        else if (resourceType == ResourceType::Texture)
        {
//            Handle<Texture> handle = Handle<Texture>::UnPack(packedHandle);
//
//            if (m_TexturePool.Release(handle))
//            {
//                DeleteTexture(handle);
//            }
        }
    }
}


