#include "MetalRenderer.h"

namespace HBL2
{
    thread_local MtlUploadContext MetalRenderer::s_UploadContext;

    void MetalRenderer::PreInitialize()
    {
        m_Device = (MetalDevice*)Device::Instance;
        m_ResourceManager = (MetalResourceManager*)ResourceManager::Instance;
        
        // Initial layer extents (Needs update on resize).
        const auto& extents = m_Device->GetMetalLayer()->drawableSize();
        
        // Commands.
        m_CommandQueue = m_Device->Get()->newMTL4CommandQueue();
        
        uint32_t index = 0;
        
        for (auto& frame : m_MtlFrames)
        {
            frame.MainCommandBuffer = m_Device->Get()->newCommandBuffer();
            frame.ImGuiCommandBuffer = m_Device->Get()->newCommandBuffer();
            frame.CommandAllocator = m_Device->Get()->newCommandAllocator();
            
            m_MainCommandBuffers[index] = MetalCommandBuffer({
                .type = CommandBufferType::MAIN,
                .commandBuffer = frame.MainCommandBuffer,
            });

            m_ImGuiCommandBuffers[index] = MetalCommandBuffer({
                .type = CommandBufferType::UI,
                .commandBuffer = frame.ImGuiCommandBuffer,
            });
            
            auto* argTableDesc = MTL4::ArgumentTableDescriptor::alloc();
            argTableDesc->setMaxBufferBindCount(16);
            argTableDesc->setMaxTextureBindCount(16);
            argTableDesc->setMaxSamplerStateBindCount(16);
            frame.GlobalArgumentTable = m_Device->Get()->newArgumentTable(argTableDesc, /* error */ nullptr);
            argTableDesc->release();
            
            index++;
        }
        
        // Residency set.
        auto* residencySetDesc = MTL::ResidencySetDescriptor::alloc();
        m_ResidencySet = m_Device->Get()->newResidencySet(residencySetDesc, nullptr);
        residencySetDesc->release();
        
        m_CommandQueue->addResidencySet(m_ResidencySet);
        m_CommandQueue->addResidencySet(m_Device->GetMetalLayer()->residencySet());
        
        // Synchronization (shared events are similar to vulkan fences).
        m_FrameAvailableSharedEvent = m_Device->Get()->newSharedEvent();
        m_FrameAvailableSharedEvent->setSignaledValue(0);
        
        // Create UploadContext Commands for this thread.
        CreateUploadContextCommands();
        
        // Create depth textures.
        for (auto& depthTexture : m_DepthTextures)
        {
            depthTexture = m_ResourceManager->CreateTexture({
                .debugName = "swapchain-depth-image",
                .dimensions = { extents.width, extents.height, 1 },
                .format = Format::D32_FLOAT,
                .internalFormat = Format::D32_FLOAT,
                .usage = TextureUsage::DEPTH_STENCIL,
                .aspect = TextureAspect::DEPTH,
                .createSampler = false,
            });
            
            MetalTexture* mtlDepthTexture = m_ResourceManager->GetTexture(depthTexture);
            m_ResidencySet->addAllocation(mtlDepthTexture->Texture);
        }
        
        // Create render passes.
        uint32_t frameIndex = 0;
        
        Handle<RenderPassLayout> renderPassLayout = m_ResourceManager->CreateRenderPassLayout({
            .debugName = "main-renderpass-layout",
            .depthTargetFormat = Format::D32_FLOAT,
            .subPasses = {
                { .depthTarget = true, .colorTargets = 1, },
            },
        });
        
        // Render passes for main rendering.
        for (auto renderPassHandle : m_RenderPasses)
        {
            renderPassHandle = ResourceManager::Instance->CreateRenderPass({
                .debugName = "main-renderpass",
                .layout = renderPassLayout,
                .depthTarget = {
                    .loadOp = LoadOperation::CLEAR,
                    .storeOp = StoreOperation::STORE,
                    .stencilLoadOp = LoadOperation::DONT_CARE,
                    .stencilStoreOp = StoreOperation::DONT_CARE,
                    .prevUsage = TextureLayout::UNDEFINED,
                    .nextUsage = TextureLayout::DEPTH_STENCIL,
                },
                .colorTargets = {
                    {
                        .loadOp = LoadOperation::CLEAR,
                        .storeOp = StoreOperation::STORE,
                        .prevUsage = TextureLayout::UNDEFINED,
                        .nextUsage = TextureLayout::RENDER_ATTACHMENT,
                    },
                },
                .frameBufferDesc = {
                    .width = (uint32_t)extents.width,
                    .height = (uint32_t)extents.height,
                    .depthTarget = m_DepthTextures[frameIndex],
                    .colorTargets = { {} }, // Empty since it requires surface texture, aquired in begin frame.
                }
            });
            
            frameIndex++;
        }
        
        frameIndex = 0;
        
        // Render passes for imgui rendering.
        for (auto renderPassHandle : m_ImGuiRenderPasses)
        {
            renderPassHandle = m_ResourceManager->CreateRenderPass({
                .debugName = "imgui-renderpass",
                .layout = renderPassLayout,
                .depthTarget = {
                    .loadOp = LoadOperation::LOAD,
                    .storeOp = StoreOperation::STORE,
                    .stencilLoadOp = LoadOperation::DONT_CARE,
                    .stencilStoreOp = StoreOperation::DONT_CARE,
                    .prevUsage = TextureLayout::DEPTH_STENCIL,
                    .nextUsage = TextureLayout::DEPTH_STENCIL,
                },
                .colorTargets = {
                    {
                        .loadOp = LoadOperation::LOAD,
                        .storeOp = StoreOperation::STORE,
                        .prevUsage = TextureLayout::RENDER_ATTACHMENT,
                        .nextUsage = TextureLayout::PRESENT,
                    },
                },
                .frameBufferDesc = {
                    .width = (uint32_t)extents.width,
                    .height = (uint32_t)extents.height,
                    .depthTarget = m_DepthTextures[frameIndex],
                    .colorTargets = { {} }, // Empty since it requires surface texture, aquired in begin frame.
                }
            });
            
            frameIndex++;
        }
        
        // Render passes for offscreen rendering.
        // We only need that to pass the VkRenderPass to the shader creation in the EditorAssetManager flow.
        // So no need to create a FrameBuffer here at all or have it per frame in flight.
        m_RenderingRenderPass = m_ResourceManager->CreateRenderPass({
            .debugName = "rendering-renderpass",
            .layout = renderPassLayout,
            .depthTarget = {
                .loadOp = LoadOperation::CLEAR,
                .storeOp = StoreOperation::STORE,
                .stencilLoadOp = LoadOperation::DONT_CARE,
                .stencilStoreOp = StoreOperation::DONT_CARE,
                .prevUsage = TextureLayout::UNDEFINED,
                .nextUsage = TextureLayout::DEPTH_STENCIL,
            },
            .colorTargets = {
                {
                    .format = Format::RGBA16_FLOAT,
                    .loadOp = LoadOperation::CLEAR,
                    .storeOp = StoreOperation::STORE,
                    .prevUsage = TextureLayout::UNDEFINED,
                    .nextUsage = TextureLayout::RENDER_ATTACHMENT,
                },
            },
        });
    }

    void MetalRenderer::PostInitialize()
    {
        
    }

    void MetalRenderer::BeginFrame()
    {
        // Attach surface color target texture to main and imgui render passes.
        CA::MetalDrawable* surface = m_Device->GetMetalLayer()->nextDrawable();
        
        MetalRenderPass* mainRp = m_ResourceManager->GetRenderPass(GetMainRenderPass());
        mainRp->SetColorTarget(0, surface->texture());
        
        MetalRenderPass* imguiRp = m_ResourceManager->GetRenderPass(GetImGuiRenderPass());
        imguiRp->SetColorTarget(0, surface->texture());
    }

    void MetalRenderer::EndFrame()
    {
        
    }

    void MetalRenderer::Present()
    {
        
    }

    void MetalRenderer::Clean()
    {
        
    }

    CommandBuffer* MetalRenderer::BeginCommandRecording(CommandBufferType type)
    {
        return nullptr;
    }

    void MetalRenderer::MakeResident(std::initializer_list<MTL::Allocation*> resources)
    {
        std::lock_guard lock(m_ResidencyMutex);
        
        for (auto* r : resources)
        {
            m_ResidencySet->addAllocation(r);
        }
        
        m_ResidencySet->commit();
    }

    void MetalRenderer::RemoveResident(MTL::Allocation* resource)
    {
        std::lock_guard lock(m_ResidencyMutex);
        m_ResidencySet->removeAllocation(resource);
        m_ResidencySet->commit();
    }

    void MetalRenderer::ImmediateSubmit(const std::function<void(MTL4::ComputeCommandEncoder*)>& fn)
    {
        if (s_UploadContext.CommandBuffer == nullptr)
        {
            CreateUploadContextCommands();
        }

        s_UploadContext.Allocator->reset();
        s_UploadContext.CommandBuffer->beginCommandBuffer(s_UploadContext.Allocator);

        MTL4::ComputeCommandEncoder* encoder = s_UploadContext.CommandBuffer->computeCommandEncoder();
        {
            fn(encoder);
        }
        encoder->endEncoding();

        s_UploadContext.CommandBuffer->endCommandBuffer();

        m_CommandQueue->commit(&s_UploadContext.CommandBuffer, 1);

        const uint64_t waitValue = ++s_UploadContext.EventValue;
        m_CommandQueue->signalEvent(s_UploadContext.Event, waitValue);
        s_UploadContext.Event->waitUntilSignaledValue(waitValue, UINT64_MAX);
    }

    MTL::Texture* MetalRenderer::CreateDepthTexture(uint32_t width, uint32_t height, uint32_t frameIdx)
    {
        MTL::TextureDescriptor* depthDesc = MTL::TextureDescriptor::alloc()->init();
        depthDesc->setTextureType(MTL::TextureType2D);
        depthDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
        depthDesc->setWidth(width);
        depthDesc->setHeight(height);
        depthDesc->setUsage(MTL::TextureUsageRenderTarget);
        depthDesc->setStorageMode(MTL::StorageModePrivate);

        MTL::Texture* texture = m_Device->Get()->newTexture(depthDesc);
        depthDesc->release();

        const std::string label = "Depth Texture (frame " + std::to_string(frameIdx) + ")";
        texture->setLabel(NS::String::string(label.c_str(), NS::UTF8StringEncoding));

        return texture;
    }

    void MetalRenderer::CreateUploadContextCommands()
    {
        s_UploadContext.Allocator = m_Device->Get()->newCommandAllocator();
        s_UploadContext.CommandBuffer = m_Device->Get()->newCommandBuffer();
        s_UploadContext.Event = m_Device->Get()->newSharedEvent();
        s_UploadContext.Event->setSignaledValue(0);
        
        // Copy upload context to correctly capture it by value in the lambda.
        auto ctx = s_UploadContext;
        
        // Queue resources for deletion.
        {
            std::lock_guard<std::mutex> lock(m_DeletionQueueMutex);

            m_MainDeletionQueue.PushFunction([this, ctx]()
            {
                ctx.Allocator->release();
                ctx.CommandBuffer->release();
                ctx.Event->release();
            });
        }
    }
}
