#pragma once

#include "Base.h"
#include "Resources/TypeDescriptors.h"
#include "Utilities/Collections/StaticDArray.h"

#include <string>
#include <stdint.h>

namespace HBL2
{
    struct MetalRenderPassLayout
    {
        MetalRenderPassLayout() = default;
        MetalRenderPassLayout(const RenderPassLayoutDescriptor&& desc);

        const char* DebugName = "";
        Format DepthTargetFormat = Format::D32_FLOAT;
        StaticDArray<RenderPassLayoutDescriptor::SubPass, 4> SubPasses;
    };
}

