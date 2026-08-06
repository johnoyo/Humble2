#include "MetalBindGroupLayout.h"

namespace HBL2
{
    MetalBindGroupLayout::MetalBindGroupLayout(const BindGroupLayoutDescriptor&& desc)
    {
        DebugName = desc.debugName;
        CreatedFromReflection = desc.createdFromReflection;
        BufferBindings = { desc.bufferBindings.begin(), desc.bufferBindings.end() };
        TextureBindings = { desc.textureBindings.begin(), desc.textureBindings.end() };
    }

    void MetalBindGroupLayout::Destroy()
    {
    }
}
