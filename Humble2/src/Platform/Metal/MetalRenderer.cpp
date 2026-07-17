#include "MetalRenderer.h"

namespace HBL2
{
    void MetalRenderer::PreInitialize()
    {
        m_Device = (MetalDevice*)Device::Instance;
        
        // Commands.
        m_CommandQueue = m_Device->Get()->newMTL4CommandQueue();
        
        uint32_t index = 0;
        
        for (auto& frame : m_MtlFrames)
        {
            frame.MainCommandBuffer = m_Device->Get()->newCommandBuffer();
            frame.ImGuiCommandBuffer = m_Device->Get()->newCommandBuffer();
            frame.CommandAllocator = m_Device->Get()->newCommandAllocator();
            
            m_MainCommandBuffers[index] = MetalCommandBuffer({
                .type = CommandBufferType::MAIN,
                .commandBuffer = frame.MainCommandBuffer,
            });

            m_ImGuiCommandBuffers[index] = MetalCommandBuffer({
                .type = CommandBufferType::UI,
                .commandBuffer = frame.ImGuiCommandBuffer,
            });
            
            auto* argTableDesc = MTL4::ArgumentTableDescriptor::alloc();
            argTableDesc->setMaxBufferBindCount(16);
            argTableDesc->setMaxTextureBindCount(16);
            argTableDesc->setMaxSamplerStateBindCount(16);
            frame.GlobalArgumentTable = m_Device->Get()->newArgumentTable(argTableDesc, /* error */ nullptr);
            argTableDesc->release();
            
            index++;
        }
        
        // Residency set.
        auto* residencySetDesc = MTL::ResidencySetDescriptor::alloc();
        m_ResidencySet = m_Device->Get()->newResidencySet(residencySetDesc, nullptr);
        residencySetDesc->release();
        
        m_CommandQueue->addResidencySet(m_ResidencySet);
        m_CommandQueue->addResidencySet(m_Device->GetMetalLayer()->residencySet());
        
        // Synchronization (shared events are similar to vulkan fences).
        m_FrameAvailableSharedEvent = m_Device->Get()->newSharedEvent();
        m_FrameAvailableSharedEvent->setSignaledValue(0);
    }

    void MetalRenderer::PostInitialize()
    {
        
    }

    void MetalRenderer::BeginFrame()
    {
        
    }

    void MetalRenderer::EndFrame()
    {
        
    }

    void MetalRenderer::Present()
    {
        
    }

    void MetalRenderer::Clean()
    {
        
    }

    CommandBuffer* MetalRenderer::BeginCommandRecording(CommandBufferType type)
    {
        return nullptr;
    }
}
