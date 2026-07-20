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
        
        MTL::ResourceOptions resourceOptions = MtlUtils::MemoryUsageToMTLResourceOptions(desc.memoryUsage);
        
        if (desc.initialData != nullptr)
        {
            Hot->Data = desc.initialData;
            
            if (resourceOptions == MTL::ResourceStorageModePrivate)
            {
                // TODO: implement copy buffer to buffer using ImmediateSubmit.
                // Also fix the MemoryUsageToMTLResourceOptions to return MTL::ResourceStorageModePrivate on GPU_ONLY.
            }
            else
            {
                Hot->Buffer = device->Get()->newBuffer(Hot->Data, Hot->ByteSize, resourceOptions);
            }
        }
        else
        {
            Hot->Buffer = device->Get()->newBuffer(Hot->ByteSize, MtlUtils::MemoryUsageToMTLResourceOptions(desc.memoryUsage));
        }
        Hot->Buffer->setLabel(NS::String::string(Cold->DebugName, NS::UTF8StringEncoding));
        
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
