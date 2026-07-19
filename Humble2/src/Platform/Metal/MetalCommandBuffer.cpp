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
        
        m_CurrentRenderPassRenderer.Encoder = CommandBuffer->renderCommandEncoder(mtlRenderPass->PassDesc);
        
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
