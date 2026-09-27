#pragma once

#include "Base.h"
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

    struct MetalBindGroupCold
    {
        static constexpr uint32_t MaxTextureEntries = 6;
        static constexpr uint32_t MaxBufferEntries = 6;
        
        uint64_t Hash = 0;
        const char* DebugName = "";
        RefHandle<BindGroupLayout> BindGroupLayout;
        StaticDArray<BindGroupDescriptor::BufferEntry, MaxBufferEntries> Buffers;
        StaticDArray<BindGroupDescriptor::TextureEntry, MaxTextureEntries> Textures;

        void Destroy();
    };

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
