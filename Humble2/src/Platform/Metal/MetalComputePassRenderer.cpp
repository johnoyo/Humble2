#include "MetalComputePassRenderer.h"

#include "MetalResourceManager.h"

namespace HBL2
{
    void MetalComputePassRenderer::Dispatch(const Span<const HBL2::Dispatch>& dispatches)
    {
        MetalRenderer* renderer = (MetalRenderer*)Renderer::Instance;
        MetalResourceManager* rm = (MetalResourceManager*)ResourceManager::Instance;

        MTL4::ArgumentTable* argTable = renderer->GetCurrentFrame().GlobalArgumentTable;
        
        for (const auto& dispatch : dispatches)
        {
            MetalShaderHot* shader = rm->GetShaderHot(dispatch.Shader);

            uint32_t bufferIndex = 0;
            uint32_t textureIndex = 0;
            
            Encoder->setComputePipelineState((MTL::ComputePipelineState*)shader->Pso);

            if (dispatch.BindGroup.IsValid())
            {
                MetalBindGroupCold* dispatchBindGroupCold = rm->GetBindGroupCold(dispatch.BindGroup);

                for (const auto& bufferEntry : dispatchBindGroupCold->Buffers)
                {
                    MetalBufferHot* buffer = rm->GetBufferHot(bufferEntry.buffer);
                    argTable->setAddress(buffer->Buffer->gpuAddress(), bufferIndex);
                    
                    bufferIndex++;
                }
                
                for (const auto& textureEntry : dispatchBindGroupCold->Textures)
                {
                    MetalTexture* texture = rm->GetTexture(textureEntry.texture);
                    argTable->setTexture(texture->Texture->gpuResourceID(), textureIndex);
                    argTable->setSamplerState(texture->Sampler->gpuResourceID(), textureIndex);
                    
                    textureIndex++;
                }
            }
            
            Encoder->setArgumentTable(argTable);

            MTL::Size threadgroupsPerGrid = MTL::Size::Make(dispatch.ThreadGroupCount.x, dispatch.ThreadGroupCount.y, dispatch.ThreadGroupCount.z);

            MTL::Size threadsPerThreadgroup = MTL::Size::Make(16, 16, 1); // TODO: Get them from shader relflection.

            Encoder->dispatchThreadgroups(threadgroupsPerGrid, threadsPerThreadgroup);
        }
    }
}

