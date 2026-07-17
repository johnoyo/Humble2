#pragma once

#include "Renderer/RenderPassRenderer.h"

#include "MetalCommon.h"

namespace HBL2
{
    class MetalRenderPassRenderer final : public RenderPassRenderer
    {
    public:
        virtual void DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws) override;

    private:
        friend class MetalCommandBuffer;
    };
}

