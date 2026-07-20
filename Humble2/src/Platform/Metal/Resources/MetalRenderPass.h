#pragma once

#include "Base.h"
#include "Resources/TypeDescriptors.h"

#include "Platform/Metal/MetalCommon.h"

#include "Utilities/Collections/StaticDArray.h"

namespace HBL2
{
    struct MetalRenderPass
    {
        MetalRenderPass() = default;
        MetalRenderPass(const RenderPassDescriptor&& desc);
        
        void SetColorTarget(uint32_t index, MTL::Texture* target);
        void SetDepthTarget(MTL::Texture* target);
        void Destroy();
        
        const char* DebugName = "";
        MTL4::RenderPassDescriptor* PassDesc = nullptr;
        StaticDArray<MTL::PixelFormat, 4> ColorAttachmentFormats;
        MTL::PixelFormat DepthAttachmentFormat;
        uint32_t ColorAttachmentCount = 0;
        uint32_t Width = 0;
        uint32_t Height = 0;
    };
}
