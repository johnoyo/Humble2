#include "MetalCommandBuffer.h"

#include "Core/Window.h"
#include "MetalRenderer.h"
#include "MetalResourceManager.h"

namespace HBL2
{
    RenderPassRenderer* MetalCommandBuffer::BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer, Viewport&& drawArea)
    {
        return nullptr;
    }

    void MetalCommandBuffer::EndRenderPass(const RenderPassRenderer& renderPassRenderer)
    {
    }

    ComputePassRenderer* MetalCommandBuffer::BeginComputePass(const Span<const Handle<Texture>>& texturesWrite, const Span<const Handle<Buffer>>& buffersWrite)
    {
        return nullptr;
    }

    void MetalCommandBuffer::EndComputePass(const ComputePassRenderer& computePassRenderer)
    {
        return;
    }

    void MetalCommandBuffer::EndCommandRecording()
    {
        
    }

    void MetalCommandBuffer::Submit()
    {
        
    }
}
