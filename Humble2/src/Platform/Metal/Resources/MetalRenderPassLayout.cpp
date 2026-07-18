#include "MetalRenderPassLayout.h"

namespace HBL2
{
    MetalRenderPassLayout::MetalRenderPassLayout(const RenderPassLayoutDescriptor&& desc)
    {
        DebugName = desc.debugName;

        DepthTargetFormat = desc.depthTargetFormat;

        for (const auto& subPass : desc.subPasses)
        {
            SubPasses.push_back(subPass);
        }
    }
}
