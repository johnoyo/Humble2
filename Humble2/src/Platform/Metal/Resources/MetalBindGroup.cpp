#include "MetalBindGroup.h"

#include "Platform/Metal/MetalResourceManager.h"

namespace HBL2
{
    void MetalBindGroupCold::Destroy()
    {
        auto* rm = (MetalResourceManager*)ResourceManager::Instance;

        BindGroupLayout.Release();
        
        for (int i = 0; i < Buffers.size(); i++)
        {
            Buffers[i].buffer.Release();
        }

        for (int i = 0; i < Textures.size(); i++)
        {
            // Textures[i].texture.Release();
        }
        
        DebugName = nullptr;
        Hash = 0;
    }

    void MetalBindGroup::Initialize(const BindGroupDescriptor&& desc)
    {
        if (!IsValid())
        {
            return;
        }
        
        Cold->DebugName = desc.debugName;
        Cold->BindGroupLayout = desc.layout;

        HBL2_CORE_ASSERT(desc.buffers.size() <= Cold->Buffers.capacity(), "Exceeded max number of buffers in a bind group!");
        for (const auto& bufferEntry : desc.buffers)
        {
            Cold->Buffers.push_back({ bufferEntry.buffer, bufferEntry.byteOffset, bufferEntry.range });
        }

        HBL2_CORE_ASSERT(desc.textures.size() <= Cold->Textures.capacity(), "Exceeded max number of textures in a bind group!");
        for (const auto& textureEntry : desc.textures)
        {
            Cold->Textures.push_back({ textureEntry.texture, textureEntry.desiredLayout });
        }
        
        Cold->Hash = ResourceManager::Instance->GetBindGroupHash(std::forward<const BindGroupDescriptor>(desc));
    }

    void MetalBindGroup::Destroy()
    {
        if (!IsValid())
        {
            return;
        }
        
        Cold->Destroy();
    }

    bool MetalBindGroup::IsValid() const
    {
        return Hot != nullptr && Cold != nullptr;
    }
}
