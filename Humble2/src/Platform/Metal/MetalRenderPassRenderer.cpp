#include "MetalRenderPassRenderer.h"

#include "MetalDevice.h"
#include "MetalRenderer.h"
#include "MetalResourceManager.h"

namespace HBL2
{
    void MetalRenderPassRenderer::DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws)
    {
        /**
         * NOTE ON ARGUMENT TABLE UPDATES:
         * We deliberately do NOT cache buffer/texture indices between draws sharing the same material or shader.
         *
         * 1. PERSISTENCE: Metal's 'setArgumentTable' performs a point-in-time snapshot of the 'argTable' struct.
         *    To apply a new per-draw dynamic UBO offset, we must call 'setArgumentTable' every draw.
         *
         * 2. STRUCTURAL ALIGNMENT: Because the engine packs indices sequentially via a dynamic 'bufferIndex++',
         *    omitting a cached bind group would cause index misalignment on subsequent iterations.
         *
         * Therefore, we rewrite the transient table slots and rebind the table for every draw call,
         * keeping only heavy hardware states (like the PSO) cached.
         */
        
        Renderer::Instance->GetStats().DrawCalls += draws.GetCount();
        
        MetalRenderer* renderer = (MetalRenderer*)Renderer::Instance;
        MetalResourceManager* rm = (MetalResourceManager*)ResourceManager::Instance;

        MTL4::ArgumentTable* argTable = renderer->GetCurrentFrame().GlobalArgumentTable;
        uint32_t bufferIndexForGlobal = 0;
        uint32_t textureIndexForGlobal = 0;
        
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
                
                if (globalDraw.GlobalBufferOffset != UINT32_MAX)
                {
                    argTable->setAddress(buffer->Buffer->gpuAddress() + globalDraw.GlobalBufferOffset, bufferIndexForGlobal);
                }
                else
                {
                    argTable->setAddress(buffer->Buffer->gpuAddress(), bufferIndexForGlobal);
                }
                
                bufferIndexForGlobal++;
            }
            
            for (const auto& textureEntry : globalBindGroupCold->Textures)
            {
                MetalTexture* texture = rm->GetTexture(textureEntry.texture);
                argTable->setTexture(texture->Texture->gpuResourceID(), textureIndexForGlobal);
                argTable->setSamplerState(texture->Sampler->gpuResourceID(), textureIndexForGlobal);
                textureIndexForGlobal++;
            }
        }
        
        Handle<Shader> prevShader;
        uint64_t prevVariantHash = 0;
        MTL::PrimitiveType topology = MTL::PrimitiveTypeTriangle;
        
        for (const auto& draw : draws.GetDraws())
        {
            uint32_t bufferIndex = bufferIndexForGlobal;
            uint32_t textureIndex = textureIndexForGlobal;

            MetalShaderHot* shader = rm->GetShaderHot(draw.Shader);

            if (prevVariantHash != draw.VariantHandle || prevShader != draw.Shader)
            {
                Encoder->setRenderPipelineState((MTL::RenderPipelineState*)shader->Pso);
                Encoder->setDepthStencilState(shader->DepthStencilState);
                
                auto variant = ShaderDescriptor::RenderPipeline::PackedVariant::FromKey(draw.VariantHandle);
                
                MTL::Winding windingOrder = MtlUtils::FrontFaceToMTLWinding((FrontFace)variant.frontFace);
                MTL::CullMode cullMode = MtlUtils::CullModeToMTLCullMode((CullMode)variant.cullMode);
                MTL::TriangleFillMode fillMode = MtlUtils::PolygonModeToMTLTriangleFillMode((PolygonMode)variant.polygonMode);
                
                Encoder->setFrontFacingWinding(windingOrder);
                Encoder->setCullMode(cullMode);
                Encoder->setTriangleFillMode(fillMode);
                
                topology = MtlUtils::TopologyToMTLPrimitiveType((Topology)variant.topology);
                
                prevVariantHash = draw.VariantHandle;
                prevShader = draw.Shader;
            }
            
            // Bind global shader descriptor set for custom per frame data.
            if (shader->ShaderBindGroup.IsValid())
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
            }
            
            // Bind the per material bind group.
            if (draw.MaterialBindGroup.IsValid())
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
            }
            
            // Bind the vertex buffer if needed.
            MetalBufferHot* vertexBuffer = rm->GetBufferHot(draw.VertexBuffer);
            argTable->setAddress(vertexBuffer->Buffer->gpuAddress(), VERTEX_BUFFER_BINDING_IDX);

            // Bind the dynamic uniform buffer in the appropriate offset.
            if (draw.BindGroup.IsValid())
            {
                MetalBindGroupCold* drawBindGroupCold = rm->GetBindGroupCold(draw.BindGroup);
                
                // Per draw bind group should only ever hold the one bump-allocated UBO.
                MetalBufferHot* buffer = rm->GetBufferHot(drawBindGroupCold->Buffers[0].buffer);
                if (globalDraw.UsesDynamicOffset)
                {
                    argTable->setAddress(buffer->Buffer->gpuAddress() + draw.Offset, bufferIndex);
                }
                else
                {
                    argTable->setAddress(buffer->Buffer->gpuAddress(), bufferIndex);
                }
                bufferIndex++;
            }
            
            // Metal takes a point-in-time snapshot of whatever memory addresses and texture IDs are sitting inside that table.
            Encoder->setArgumentTable(argTable, MTL::RenderStageVertex | MTL::RenderStageFragment);
            
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

