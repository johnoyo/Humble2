#pragma once

#include "Renderer/ComputePassRenderer.h"

#include "Platform/Metal/MetalCommon.h"

namespace HBL2
{
    class MetalComputePassRenderer final : public ComputePassRenderer
    {
    public:
        virtual void Dispatch(const Span<const HBL2::Dispatch>& dispatches) override;

        MTL4::ComputeCommandEncoder* Encoder = nullptr;
        
    private:
        MTL4::CommandBuffer* m_CommandBuffer = nullptr;
        friend class MetalCommandBuffer;
    };
}

