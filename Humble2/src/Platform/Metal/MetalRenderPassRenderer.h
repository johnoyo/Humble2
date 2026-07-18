#pragma once

#include "Renderer/RenderPassRenderer.h"

#include "MetalCommon.h"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

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

