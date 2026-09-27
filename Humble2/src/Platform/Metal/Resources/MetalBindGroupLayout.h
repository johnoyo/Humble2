#pragma once

#include "Base.h"
#include "Resources/TypeDescriptors.h"

#include "Platform/Metal/MetalDevice.h"

#include "Platform/Metal/MetalCommon.h"

namespace HBL2
{
    struct MetalBindGroupLayout
    {
        static constexpr uint32_t MaxTextureEntries = 6;
        static constexpr uint32_t MaxBufferEntries = 6;
        
        MetalBindGroupLayout() = default;
        MetalBindGroupLayout(const BindGroupLayoutDescriptor&& desc);

        void Destroy();

        uint64_t Hash = 0;
        const char* DebugName = "";
        StaticDArray<BindGroupLayoutDescriptor::BufferBinding, MaxBufferEntries> BufferBindings;
        StaticDArray<BindGroupLayoutDescriptor::TextureBinding, MaxTextureEntries> TextureBindings;
    };
}
