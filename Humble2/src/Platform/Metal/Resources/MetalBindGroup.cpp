#include "MetalBindGroup.h"

#include "Platform/Metal/MetalResourceManager.h"

namespace HBL2
{
    void MetalBindGroupCold::Destroy()
    {
        auto* rm = (MetalResourceManager*)ResourceManager::Instance;

        for (int i = 0; i < Buffers.size(); i++)
        {
            // If the range is not 0, this means its a dynamic uniform buffer meaning that is shared across bindgroup, so do not delete.
            if (Buffers[i].range == 0)
            {
                rm->DeleteBuffer(Buffers[i].buffer);
            }
        }
    }

    void MetalBindGroup::Initialize(const BindGroupDescriptor &&desc)
    {
        if (!IsValid())
        {
            return;
        }
        
        Cold->DebugName = desc.debugName;

        Cold->Buffers = { desc.buffers.begin(), desc.buffers.end() };
        Cold->Textures = { desc.textures.begin(), desc.textures.end() };
        Cold->BindGroupLayout = desc.layout;
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
