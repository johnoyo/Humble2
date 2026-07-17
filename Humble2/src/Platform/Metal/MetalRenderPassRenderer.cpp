#include "MetalRenderPassRenderer.h"

#include "MetalDevice.h"
#include "MetalRenderer.h"
#include "MetalResourceManager.h"

namespace HBL2
{
    void MetalRenderPassRenderer::DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws)
    {
        Renderer::Instance->GetStats().DrawCalls += draws.GetCount();
    }
}

