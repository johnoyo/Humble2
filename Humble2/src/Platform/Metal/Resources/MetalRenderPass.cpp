#include "MetalRenderPass.h"

#include "Platform/Metal/MetalResourceManager.h"

namespace HBL2
{
    MetalRenderPass::MetalRenderPass(const RenderPassDescriptor&& desc)
    {
        MetalResourceManager* rm = (MetalResourceManager*)ResourceManager::Instance;
        
        Width = desc.frameBufferDesc.width;
        Height = desc.frameBufferDesc.height;
        PassDesc = MTL4::RenderPassDescriptor::alloc()->init();
        ColorAttachmentCount = (uint32_t)desc.colorTargets.Size();
        ColorAttachmentFormats.clear();
        
        uint32_t colorTargetIndex = 0;
        
        for (auto colorTarget : desc.frameBufferDesc.colorTargets)
        {
            const auto& colorTargetDesc = desc.colorTargets[colorTargetIndex];
            const auto& clearColor = colorTargetDesc.clearColor;
            
            MTL::RenderPassColorAttachmentDescriptor* colorAttachment = PassDesc->colorAttachments()->object(colorTargetIndex);
            colorAttachment->setLoadAction(MtlUtils::LoadOperationToMTLLoadAction(colorTargetDesc.loadOp));
            colorAttachment->setStoreAction(MtlUtils::StoreOperationMTLStoreAction(colorTargetDesc.storeOp));
            colorAttachment->setClearColor(MTL::ClearColor::Make(clearColor.r, clearColor.g, clearColor.b, clearColor.a));
            
            MetalTexture* colorTexture = rm->GetTexture(colorTarget);
            if (colorTexture != nullptr)
            {
                colorAttachment->setTexture(colorTexture->Texture);
            }
            
            colorTargetIndex++;
        }
        
        for (auto colorTargetDesc : desc.colorTargets)
        {
            ColorAttachmentFormats.push_back(MtlUtils::FormatToMTLPixelFormat(colorTargetDesc.format));
        }
        
        MTL::RenderPassDepthAttachmentDescriptor* depthAttachment = PassDesc->depthAttachment();
        depthAttachment->setLoadAction(MtlUtils::LoadOperationToMTLLoadAction(desc.depthTarget.loadOp));
        depthAttachment->setStoreAction(MtlUtils::StoreOperationMTLStoreAction(desc.depthTarget.storeOp));
        depthAttachment->setClearDepth(desc.depthTarget.clearZ);
        
        MetalTexture* depthTexture = rm->GetTexture(desc.frameBufferDesc.depthTarget);
        if (depthTexture != nullptr)
        {
            depthAttachment->setTexture(depthTexture->Texture);
        }
        
        MetalRenderPassLayout* rpl = rm->GetRenderPassLayout(desc.layout);
        if (rpl != nullptr)
        {
            DepthAttachmentFormat = MtlUtils::FormatToMTLPixelFormat(rpl->DepthTargetFormat);
        }
    }

    void MetalRenderPass::SetColorTarget(uint32_t index, MTL::Texture* target)
    {
        PassDesc->colorAttachments()->object(index)->setTexture(target);
    }

    void MetalRenderPass::SetDepthTarget(MTL::Texture* target)
    {
        PassDesc->depthAttachment()->setTexture(target);
    }

    void MetalRenderPass::Destroy()
    {
        PassDesc->release();
    }
}
