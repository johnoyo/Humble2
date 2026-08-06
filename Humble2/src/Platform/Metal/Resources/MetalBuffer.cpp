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
                // Shared staging buffer, CPU-writable, holding the initial data.
                MTL::Buffer* stagingBuffer = device->Get()->newBuffer(Hot->Data, Hot->ByteSize, MTL::ResourceStorageModeShared);
                stagingBuffer->setLabel(NS::String::string("StagingBuffer", NS::UTF8StringEncoding));

                // GPU-only destination buffer.
                Hot->Buffer = device->Get()->newBuffer(Hot->ByteSize, resourceOptions);
                Hot->Buffer->setLabel(NS::String::string(Cold->DebugName, NS::UTF8StringEncoding));
                
                renderer->MakeResident({ Hot->Buffer, stagingBuffer });

                renderer->ImmediateSubmit([=, this](MTL4::ComputeCommandEncoder* encoder)
                {
                    encoder->copyFromBuffer(stagingBuffer, 0, Hot->Buffer, 0, Hot->ByteSize);
                });

                renderer->RemoveResident(stagingBuffer);
                stagingBuffer->release();
                
                return;
            }
            else
            {
                Hot->Buffer = device->Get()->newBuffer(Hot->Data, Hot->ByteSize, resourceOptions);
            }
        }
        else
        {
            Hot->Buffer = device->Get()->newBuffer(Hot->ByteSize, resourceOptions);
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
