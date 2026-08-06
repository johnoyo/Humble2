#pragma once

#include "Renderer/RenderPassRenderer.h"

#include "MetalCommon.h"

namespace HBL2
{
    class MetalRenderPassRenderer final : public RenderPassRenderer
    {
    public:
        virtual void DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws) override;
        
        MTL4::RenderCommandEncoder* Encoder = nullptr;
        
    private:
        MTL4::CommandBuffer* m_CommandBuffer = nullptr;
        
        friend class MetalCommandBuffer;
    };
}

