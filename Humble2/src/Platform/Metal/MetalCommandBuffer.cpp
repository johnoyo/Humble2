#include "MetalCommandBuffer.h"

#include "Core/Window.h"
#include "MetalRenderer.h"
#include "MetalResourceManager.h"

namespace HBL2
{
    MetalCommandBuffer::MetalCommandBuffer(const MtlCommandBufferCreateInfo&& commandBufferCreateInfo)
        : m_Type(commandBufferCreateInfo.type), CommandBuffer(commandBufferCreateInfo.commandBuffer)
    {
        
    }

    RenderPassRenderer* MetalCommandBuffer::BeginRenderPass(Handle<RenderPass> renderPass, Viewport&& drawArea)
    {
        MetalResourceManager* rm = (MetalResourceManager*)ResourceManager::Instance;

        m_CurrentRenderPassRenderer.m_CommandBuffer = CommandBuffer;

        if (!renderPass.IsValid())
        {
            return &m_CurrentRenderPassRenderer;
        }
        
        MetalRenderPass* mtlRenderPass = rm->GetRenderPass(renderPass);

        if (!drawArea.IsValid())
        {
            drawArea =
            {
                0,
                0,
                mtlRenderPass->Width,
                mtlRenderPass->Height
            };
        }
        
        m_CurrentRenderPassRenderer.Encoder = CommandBuffer->renderCommandEncoder(mtlRenderPass->PassDesc);
        
        // Viewport
        MTL::Viewport viewport;
        viewport.originX = drawArea.x;
        viewport.originY = drawArea.y;
        viewport.width   = drawArea.width;
        viewport.height  = drawArea.height;
        viewport.znear   = 0.0;
        viewport.zfar    = 1.0;

        m_CurrentRenderPassRenderer.Encoder->setViewport(viewport);

        // Scissor
        MTL::ScissorRect scissor;
        scissor.x      = static_cast<NS::UInteger>(drawArea.x);
        scissor.y      = static_cast<NS::UInteger>(drawArea.y);
        scissor.width  = static_cast<NS::UInteger>(drawArea.width);
        scissor.height = static_cast<NS::UInteger>(drawArea.height);

        m_CurrentRenderPassRenderer.Encoder->setScissorRect(scissor);
        
        return &m_CurrentRenderPassRenderer;
    }

    void MetalCommandBuffer::EndRenderPass(const RenderPassRenderer& renderPassRenderer)
    {
        ((MetalRenderPassRenderer*)&renderPassRenderer)->Encoder->endEncoding();
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
        CommandBuffer->endCommandBuffer();
    }

    void MetalCommandBuffer::Submit()
    {
        MetalRenderer* renderer = (MetalRenderer*)Renderer::Instance;
        
        renderer->GetCommandQueue()->wait(renderer->GetCurrentSurface());
        renderer->GetCommandQueue()->commit(&CommandBuffer, 1);
    }
}
