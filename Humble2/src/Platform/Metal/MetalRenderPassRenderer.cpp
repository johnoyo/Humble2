#include "MetalRenderPassRenderer.h"

#include "MetalDevice.h"
#include "MetalRenderer.h"
#include "MetalResourceManager.h"

namespace HBL2
{
    void MetalRenderPassRenderer::DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws)
    {
        Renderer::Instance->GetStats().DrawCalls += draws.GetCount();
        
        MetalDevice* device = (MetalDevice*)Device::Instance;
        MetalRenderer* renderer = (MetalRenderer*)Renderer::Instance;
        MetalResourceManager* rm = (MetalResourceManager*)ResourceManager::Instance;

        MTL4::ArgumentTable* argTable = renderer->GetCurrentFrame().GlobalArgumentTable;
        uint32_t bufferIndexForGlobalBindGroup = 0;
        uint32_t textureIndexForGlobalBindGroup = 0;
        
        // Global descriptor set.
        MetalBindGroupHot* globalBindGroupHot = nullptr;
        MetalBindGroupCold* globalBindGroupCold = nullptr;

        if (globalDraw.BindGroup.IsValid())
        {
            globalBindGroupHot = rm->GetBindGroupHot(globalDraw.BindGroup);
            globalBindGroupCold = rm->GetBindGroupCold(globalDraw.BindGroup);
            
            // Map global buffers per frame data (i.e.: Camera and lighting data)
            for (const auto& bufferEntry : globalBindGroupCold->Buffers)
            {
                MetalBufferHot* buffer = rm->GetBufferHot(bufferEntry.buffer);
                memcpy(buffer->Buffer->contents(), buffer->Data, buffer->ByteSize);
                
                if (globalDraw.UsesDynamicOffset)
                {
                    argTable->setAddress(buffer->Buffer->gpuAddress() + globalDraw.GlobalBufferOffset, bufferIndexForGlobalBindGroup);
                }
                else
                {
                    argTable->setAddress(buffer->Buffer->gpuAddress(), bufferIndexForGlobalBindGroup);
                }
                
                bufferIndexForGlobalBindGroup++;
            }
            
            for (const auto& textureEntry : globalBindGroupCold->Textures)
            {
                MetalTexture* texture = rm->GetTexture(textureEntry.texture);
                argTable->setTexture(texture->Texture->gpuResourceID(), textureIndexForGlobalBindGroup);
                argTable->setSamplerState(texture->Sampler->gpuResourceID(), textureIndexForGlobalBindGroup);
                textureIndexForGlobalBindGroup++;
            }
        }
        
        Handle<Buffer> prevIndexBuffer;
        Handle<Buffer> prevVertexBuffer;
        Handle<Shader> prevShader;
        Handle<BindGroup> prevShaderBindGroup;
        Handle<BindGroup> prevMaterialBindGroup;

        uint64_t prevVariantHash = 0;
        uint64_t prevBindGroupLayoutHash = 0;

        for (const auto& draw : draws.GetDraws())
        {
            // TODO: Broken solution due to caching, if the control flow does not go into the if statement the indexing fails.
            uint32_t bufferIndex = bufferIndexForGlobalBindGroup;
            uint32_t textureIndex = textureIndexForGlobalBindGroup;
            
            auto variant = ShaderDescriptor::RenderPipeline::PackedVariant::FromKey(draw.VariantHandle);

            MetalShaderHot* shader = rm->GetShaderHot(draw.Shader);

            if (prevVariantHash != draw.VariantHandle || prevShader != draw.Shader)
            {
                Encoder->setRenderPipelineState((MTL::RenderPipelineState*)shader->Pso);
                Encoder->setDepthStencilState(shader->DepthStencilState);
                
                // Bind global shader descriptor set for custom per frame data if needed.
                if (shader->ShaderBindGroup.IsValid() && prevShaderBindGroup != shader->ShaderBindGroup)
                {
                    MetalBindGroupCold* shaderBindGroupCold = rm->GetBindGroupCold(shader->ShaderBindGroup);

                    for (const auto& bufferEntry : shaderBindGroupCold->Buffers)
                    {
                        MetalBufferHot* buffer = rm->GetBufferHot(bufferEntry.buffer);
                        argTable->setAddress(buffer->Buffer->gpuAddress(), bufferIndex);
                        bufferIndex++;
                    }
                    
                    for (const auto& textureEntry : shaderBindGroupCold->Textures)
                    {
                        MetalTexture* texture = rm->GetTexture(textureEntry.texture);
                        argTable->setTexture(texture->Texture->gpuResourceID(), textureIndex);
                        argTable->setSamplerState(texture->Sampler->gpuResourceID(), textureIndex);
                        textureIndex++;
                    }

                    prevShaderBindGroup = shader->ShaderBindGroup;
                }
            }
            
            // Bind the per material bind group if needed.
            if (draw.MaterialBindGroup.IsValid() && prevMaterialBindGroup != draw.MaterialBindGroup)
            {
                MetalBindGroupCold* materialBindGroupCold = rm->GetBindGroupCold(draw.MaterialBindGroup);

                for (const auto& bufferEntry : materialBindGroupCold->Buffers)
                {
                    MetalBufferHot* buffer = rm->GetBufferHot(bufferEntry.buffer);
                    argTable->setAddress(buffer->Buffer->gpuAddress(), bufferIndex);
                    bufferIndex++;
                }
                
                for (const auto& textureEntry : materialBindGroupCold->Textures)
                {
                    MetalTexture* texture = rm->GetTexture(textureEntry.texture);
                    argTable->setTexture(texture->Texture->gpuResourceID(), textureIndex);
                    argTable->setSamplerState(texture->Sampler->gpuResourceID(), textureIndex);
                    textureIndex++;
                }
                
                prevMaterialBindGroup = draw.MaterialBindGroup;
            }
            
            // Bind the vertex buffer if needed.
            if (prevVertexBuffer != draw.VertexBuffer)
            {
                MetalBufferHot* vertexBuffer = rm->GetBufferHot(draw.VertexBuffer);
                argTable->setAddress(vertexBuffer->Buffer->gpuAddress(), VERTEX_BUFFER_BINDING_IDX);
                prevVertexBuffer = draw.VertexBuffer;
            }

            // Bind the dynamic uniform buffer in the appropriate offset.
            if (draw.BindGroup.IsValid())
            {
                MetalBindGroupCold* drawBindGroupCold = rm->GetBindGroupCold(draw.BindGroup);

                // Per draw bind group should only ever hold the one bump-allocated UBO.
                MetalBufferHot* buffer = rm->GetBufferHot(drawBindGroupCold->Buffers[0].buffer);
                argTable->setAddress(buffer->Buffer->gpuAddress() + draw.Offset, bufferIndex);
                bufferIndex++;
            }
            
            // Get topology.
            MTL::PrimitiveType topology = MtlUtils::TopologyToMTLPrimitiveType((Topology)variant.topology);
            
            // Draw the mesh accordingly.
            if (draw.IndexBuffer.IsValid())
            {
                MetalBufferHot* indexBuffer = rm->GetBufferHot(draw.IndexBuffer);
                
                Encoder->drawIndexedPrimitives(topology, NS::UInteger(draw.IndexCount), MTL::IndexTypeUInt32,
                                               indexBuffer->Buffer->gpuAddress() + draw.IndexOffset,
                                               NS::UInteger(draw.IndexCount * sizeof(uint32_t)), NS::UInteger(draw.InstanceCount),
                                               NS::UInteger(draw.VertexOffset), NS::UInteger(draw.InstanceOffset));
            }
            else
            {
                Encoder->drawPrimitives(topology, draw.VertexOffset, draw.VertexCount, draw.InstanceCount, draw.InstanceOffset);
            }
        }
    }
}

