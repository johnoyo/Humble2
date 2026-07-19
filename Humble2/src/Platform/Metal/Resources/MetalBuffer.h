#pragma once

#include "Base.h"
#include "Resources/TypeDescriptors.h"

#include "Platform/Metal/MetalDevice.h"
#include "Platform/Metal/MetalRenderer.h"

#include "Platform/Metal/MetalCommon.h"

namespace HBL2
{
    struct MetalBufferHot
    {
        MTL::Buffer* Buffer = nullptr;
        void* Data = nullptr;
        uint32_t ByteSize = 0;

        void Destroy();
    };

    struct MetalBufferCold
    {
        const char* DebugName = "";
        uint32_t ByteOffset = 0;
    };

    struct MetalBuffer
    {
        MetalBuffer() = default;

        bool IsValid() const;

        void Initialize(const BufferDescriptor&& desc);
        void Destroy();

        MetalBufferHot* Hot;
        MetalBufferCold* Cold;
    };
}
