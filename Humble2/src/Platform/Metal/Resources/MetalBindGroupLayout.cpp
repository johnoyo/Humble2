#include "MetalBindGroupLayout.h"

#include "Resources/ResourceManager.h"

namespace HBL2
{
    MetalBindGroupLayout::MetalBindGroupLayout(const BindGroupLayoutDescriptor&& desc)
    {
        DebugName = desc.debugName;
        
        HBL2_CORE_ASSERT(desc.bufferBindings.size() < BufferBindings.capacity(), "Exceeded max number of buffer bindings in a bind group layout!");
        for (const auto& bufferBinding : desc.bufferBindings)
        {
            BufferBindings.emplace_back(bufferBinding);
        }

        HBL2_CORE_ASSERT(desc.textureBindings.size() < TextureBindings.capacity(), "Exceeded max number of texture bindings in a bind group layout!");
        for (const auto& textureBinding : desc.textureBindings)
        {
            TextureBindings.emplace_back(textureBinding);
        }
        
        Hash = ResourceManager::Instance->GetBindGroupLayoutHash(std::forward<const BindGroupLayoutDescriptor>(desc));
    }

    void MetalBindGroupLayout::Destroy()
    {
        BufferBindings.clear();
        TextureBindings.clear();
        
        DebugName = nullptr;
        Hash = 0;
    }
}
