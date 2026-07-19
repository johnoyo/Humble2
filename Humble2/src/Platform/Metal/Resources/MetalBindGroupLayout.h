#pragma once

#include "Base.h"
#include "Resources/RefCounted.h"
#include "Resources/TypeDescriptors.h"

#include "Platform/Metal/MetalDevice.h"

#include "Platform/Metal/MetalCommon.h"

namespace HBL2
{
    struct MetalBindGroupLayout : RefCounted
    {
        MetalBindGroupLayout() = default;
        MetalBindGroupLayout(const BindGroupLayoutDescriptor&& desc);

        void Destroy();

        const char* DebugName = "";
        std::vector<BindGroupLayoutDescriptor::BufferBinding> BufferBindings;
        std::vector<BindGroupLayoutDescriptor::TextureBinding> TextureBindings;
        bool CreatedFromReflection = false;
    };
}
