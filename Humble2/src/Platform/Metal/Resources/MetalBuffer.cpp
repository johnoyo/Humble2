#include "MetalBuffer.h"

namespace HBL2
{
    void MetalBufferHot::Destroy()
    {
        MetalRenderer* renderer = (MetalRenderer*)Renderer::Instance;
        
        if (Buffer != nullptr)
        {
            renderer->RemoveResident(Buffer);
            Buffer->release();
        }
    }

    void MetalBuffer::Initialize(const BufferDescriptor &&desc)
    {
        if (!IsValid())
        {
            return;
        }
        
        MetalDevice* device = (MetalDevice*)Device::Instance;
        MetalRenderer* renderer = (MetalRenderer*)Renderer::Instance;
        
        Cold->DebugName = desc.debugName;
        Hot->ByteSize = desc.byteSize;
        
        Hot->Buffer = device->Get()->newBuffer(Hot->ByteSize, MtlUtils::MemoryUsageToMTLResourceOptions(desc.memoryUsage));
        Hot->Buffer->setLabel(NS::String::string(Cold->DebugName, NS::UTF8StringEncoding));
        
        if (desc.initialData != nullptr)
        {
            Hot->Data = desc.initialData;
            memcpy(Hot->Buffer->contents(), Hot->Data, Hot->ByteSize);
        }
        
        renderer->MakeResident({ Hot->Buffer });
    }

    void MetalBuffer::Destroy()
    {
        if (!IsValid())
        {
            return;
        }
        
        Hot->Destroy();
    }

    bool MetalBuffer::IsValid() const
    {
        return Hot != nullptr && Cold != nullptr;
    }
}
