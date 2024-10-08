#include "OpenGLCommandBuffer.h"

namespace HBL2
{
    RenderPassRenderer* OpenGLCommandBuffer::BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer)
    {
        return &m_CurrentRenderPassRenderer;
    }

    void OpenGLCommandBuffer::EndRenderPass(const RenderPassRenderer& renderPassRenderer)
    {
    }

    void OpenGLCommandBuffer::Submit()
    {
    }
}
