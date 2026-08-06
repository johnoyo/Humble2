#include "MetalCommandBuffer.h"

#include "Core/Window.h"
#include "MetalRenderer.h"
#include "MetalResourceManager.h"

namespace HBL2
{
    void MetalBarrierTracker::Add(MTL::Stages after, MTL::Stages before)
    {
        if (after == 0 || before == 0)
        {
            return;
        }
        
        AfterStages |= after;
        BeforeStages |= before;
        Pending = true;
    }

    void MetalBarrierTracker::Add(ResourceState oldState, ResourceState newState)
    {
        MTL::Stages oldProducer, oldConsumer, newProducer, newConsumer;
        MtlUtils::ResourceStateToMTLStagesSplit(oldState, &oldProducer, &oldConsumer);
        MtlUtils::ResourceStateToMTLStagesSplit(newState, &newProducer, &newConsumer);

        MTL::Stages after = oldProducer ? oldProducer : oldConsumer;
        MTL::Stages before = newConsumer ? newConsumer : newProducer;

        if (after == 0 || before == 0)
        {
            return;
        }

        AfterStages  |= after;
        BeforeStages |= before;
        Pending = true;
    }

    void MetalBarrierTracker::Flush(MTL4::CommandEncoder* encoder)
    {
        if (!Pending)
        {
            return;
        }
        
        encoder->barrierAfterQueueStages(AfterStages, BeforeStages, MTL4::VisibilityOptionDevice);
        
        AfterStages = 0;
        BeforeStages = 0;
        Pending = false;
    }

    void MetalBarrierTracker::FlushInline(MTL4::CommandEncoder* encoder, MTL::Stages encoderStages)
    {
        if (!Pending)
        {
            return;
        }
        
        MTL::Stages afterInEncoder = AfterStages  & encoderStages;
        MTL::Stages beforeInEncoder = BeforeStages & encoderStages;
        
        if (afterInEncoder != 0 && beforeInEncoder != 0)
        {
            encoder->barrierAfterEncoderStages(afterInEncoder, beforeInEncoder, MTL4::VisibilityOptionDevice);
        }
        
        Flush(encoder);
    }

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
        
        m_BarrierTracker.Flush(m_CurrentRenderPassRenderer.Encoder);

        m_CurrentEncoder = m_CurrentRenderPassRenderer.Encoder;
        m_CurrentEncoderStages = MTL::StageVertex | MTL::StageFragment | MTL::StageTile | MTL::StageObject;
        
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
        auto* encoder = ((MetalRenderPassRenderer*)&renderPassRenderer)->Encoder;

        // Unconditional safety barrier: back-to-back render encoders are NOT implicitly ordered
        // by Metal 4 just because they're issued in sequence.
        encoder->barrierAfterStages(MTL::StageFragment, MTL::StageFragment, MTL4::VisibilityOptionDevice);

        encoder->endEncoding();
        
        m_CurrentEncoder = nullptr;
        m_CurrentEncoderStages = 0;
    }

    ComputePassRenderer* MetalCommandBuffer::BeginComputePass(const Span<const Handle<Texture>>& texturesWrite, const Span<const Handle<Buffer>>& buffersWrite)
    {
        m_CurrentComputePassRenderer.Encoder = CommandBuffer->computeCommandEncoder();
        
        m_BarrierTracker.Flush(m_CurrentComputePassRenderer.Encoder);

        m_CurrentEncoder = m_CurrentComputePassRenderer.Encoder;
        m_CurrentEncoderStages = MTL::StageDispatch | MTL::StageBlit;
        
        m_TexturesWrite = texturesWrite;
        m_BuffersWrite = buffersWrite;
        
        return &m_CurrentComputePassRenderer;
    }

    void MetalCommandBuffer::EndComputePass(const ComputePassRenderer& computePassRenderer)
    {
        if (m_TexturesWrite.Size() != 0 || m_BuffersWrite.Size() != 0)
        {
            m_BarrierTracker.Add(MTL::StageDispatch | MTL::StageBlit, MTL::StageVertex | MTL::StageFragment | MTL::StageDispatch | MTL::StageBlit);
        }
        
        ((MetalComputePassRenderer*)&computePassRenderer)->Encoder->endEncoding();
        
        m_CurrentEncoder = nullptr;
        m_CurrentEncoderStages = 0;
    }

    void MetalCommandBuffer::EndCommandRecording()
    {
        CommandBuffer->endCommandBuffer();
    }

    void MetalCommandBuffer::Submit()
    {
        MetalRenderer* renderer = (MetalRenderer*)Renderer::Instance;
        
        if (m_Type == CommandBufferType::MAIN)
        {
            renderer->EnsureDrawableAcquired();
            renderer->GetCommandQueue()->wait(renderer->GetCurrentSurface());
            
            renderer->GetCommandQueue()->commit(&CommandBuffer, 1);
            
            return;
        }
        
        if (m_Type == CommandBufferType::UI)
        {
            MTL4::CommitOptions* options = MTL4::CommitOptions::alloc()->init();

            dispatch_semaphore_t sema = renderer->GetFrameSemaphore();
            options->addFeedbackHandler([sema](MTL4::CommitFeedback* pFeedback)
            {
                dispatch_semaphore_signal(sema);
            });
            
            renderer->GetCommandQueue()->commit(&CommandBuffer, 1, options);
            
            options->release();
        }
    }

    void MetalCommandBuffer::AddPendingBarrier(ResourceState oldState, ResourceState newState)
    {
        m_BarrierTracker.Add(oldState, newState);
        
        if (m_CurrentEncoder)
        {
            // Pass already open so flush now, not next Begin.
            m_BarrierTracker.FlushInline(m_CurrentEncoder, m_CurrentEncoderStages);
        }
    }
}
