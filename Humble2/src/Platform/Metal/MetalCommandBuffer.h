#pragma once

#include "Renderer/CommandBuffer.h"
#include "MetalRenderPassRenderer.h"
#include "MetalComputePassRenderer.h"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

namespace HBL2
{
    struct PendingBarrier
    {
        MTL::Stages After;
        MTL::Stages Before;
    };

    struct MtlCommandBufferCreateInfo
    {
        CommandBufferType type;
        MTL4::CommandBuffer* commandBuffer = nullptr;
    };

    class MetalCommandBuffer final : public CommandBuffer
    {
    public:
        MetalCommandBuffer() = default;
        MetalCommandBuffer(const MtlCommandBufferCreateInfo&& commandBufferCreateInfo);
        
        virtual RenderPassRenderer* BeginRenderPass(Handle<RenderPass> renderPass, Viewport&& drawArea = {}) override;
        virtual void EndRenderPass(const RenderPassRenderer& renderPassRenderer) override;

        virtual ComputePassRenderer* BeginComputePass(const Span<const Handle<Texture>>& texturesWrite, const Span<const Handle<Buffer>>& buffersWrite) override;
        virtual void EndComputePass(const ComputePassRenderer& computePassRenderer) override;

        virtual void EndCommandRecording() override;
        virtual void Submit() override;
        
        MTL4::CommandBuffer* CommandBuffer = nullptr;
        
        void AddPendingBarrier(const PendingBarrier& pendingbarrier) { m_PendingBarriers.push_back(pendingbarrier); }
        
    private:
        CommandBufferType m_Type;
        MetalRenderPassRenderer m_CurrentRenderPassRenderer;
        MetalComputePassRenderer m_CurrentComputePassRenderer;
        std::vector<PendingBarrier> m_PendingBarriers;
    };
}
