#pragma once

#include "Renderer/ComputePassRenderer.h"

#include "Platform/Metal/MetalCommon.h"

namespace HBL2
{
    class MetalComputePassRenderer final : public ComputePassRenderer
    {
    public:
        virtual void Dispatch(const Span<const HBL2::Dispatch>& dispatches) override;

    private:
        friend class MetalCommandBuffer;
    };
}

