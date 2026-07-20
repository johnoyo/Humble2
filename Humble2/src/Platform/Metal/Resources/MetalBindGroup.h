#pragma once

#include "Base.h"
#include "Resources/RefCounted.h"
#include "Resources/TypeDescriptors.h"

#include "Platform/Metal/MetalDevice.h"
#include "Platform/Metal/MetalRenderer.h"

#include "Platform/Metal/MetalCommon.h"

namespace HBL2
{
    struct MetalBindGroupHot
    {
        void* DescriptorSet = nullptr;
    };

    struct MetalBindGroupCold : public RefCounted
    {
        const char* DebugName = "";
        Handle<BindGroupLayout> BindGroupLayout;
        std::vector<BindGroupDescriptor::TextureEntry> Textures;
        std::vector<BindGroupDescriptor::BufferEntry> Buffers;

        void Destroy();
    };

    // Helper struct for centralised operations on hot and cold data.
    // NOTE: Use with SplitPool::Get to retrieve the Hot and Cold data from the pool.
    struct MetalBindGroup
    {
        MetalBindGroup() = default;

        bool IsValid() const;

        void Initialize(const BindGroupDescriptor&& desc);
        void Destroy();

        MetalBindGroupHot* Hot = nullptr;
        MetalBindGroupCold* Cold = nullptr;
    };
}
