#include "MetalRenderer.h"

#include "Platform/Metal/MetalResourceManager.h"

namespace HBL2
{
    thread_local MtlUploadContext MetalRenderer::s_UploadContext;

    void MetalRenderer::PreInitialize()
    {
        m_GraphicsAPI = GraphicsAPI::METAL;
        
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
            
            {
                auto* argTableDesc = MTL4::ArgumentTableDescriptor::alloc();
                argTableDesc->setMaxBufferBindCount(16);
                argTableDesc->setMaxTextureBindCount(16);
                argTableDesc->setMaxSamplerStateBindCount(16);
                frame.GlobalArgumentTable = m_Device->Get()->newArgumentTable(argTableDesc, /* error */ nullptr);
                argTableDesc->release();
            }
            
            {
                auto* argTableDesc = MTL4::ArgumentTableDescriptor::alloc();
                argTableDesc->setMaxBufferBindCount(16);
                argTableDesc->setMaxTextureBindCount(16);
                argTableDesc->setMaxSamplerStateBindCount(16);
                frame.GlobalComputeArgumentTable = m_Device->Get()->newArgumentTable(argTableDesc, /* error */ nullptr);
                argTableDesc->release();
            }
            
            index++;
        }
        
        // Residency set.
        auto* residencySetDesc = MTL::ResidencySetDescriptor::alloc();
        m_ResidencySet = m_Device->Get()->newResidencySet(residencySetDesc, nullptr);
        residencySetDesc->release();
        
        m_CommandQueue->addResidencySet(m_ResidencySet);
        m_CommandQueue->addResidencySet(m_Device->GetMetalLayer()->residencySet());
        
        // Semaphore initialized with capacity = 2 (allows 2 frames in-flight)
        m_FrameSemaphore = dispatch_semaphore_create(FRAME_OVERLAP);
        
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
        }
        
        // Create render passes.
        CreateRenderPasses();
    }

    void MetalRenderer::PostInitialize()
    {
        // Register event to update the drawble size on resize.
        EventDispatcher::Get().Register<FramebufferSizeEvent>([this](const FramebufferSizeEvent& e)
        {
            Resize(e.Width, e.Height);
        });
        
        m_CaptureManager = MTL::CaptureManager::sharedCaptureManager();
    }

    CommandBuffer* MetalRenderer::BeginFrame()
    {
        m_AutoReleasePool = NS::AutoreleasePool::alloc()->init();
        
        // Signal that frame is ready.
        dispatch_semaphore_wait(m_FrameSemaphore, DISPATCH_TIME_FOREVER);
        
        // Flush any pending deletions that occured in the frames before the current one.
        m_ResourceManager->Flush(m_FrameNumber.load());
        
        // Reset frames command allocator.
        GetCurrentFrame().CommandAllocator->reset();
        
        // Invalidate surface, it will be aquired late, before rendering.
        m_SurfaceRef = nullptr;
        
        SwapAndResetStats();

        CommandBuffer* commandBuffer = BeginCommandRecording(CommandBufferType::MAIN);
        return commandBuffer;
    }

    void MetalRenderer::EndFrame(CommandBuffer* commandBuffer)
    {
        commandBuffer->EndCommandRecording();
        commandBuffer->Submit();
    }

    void MetalRenderer::Present()
    {
        if (!m_SurfaceRef)
        {
            m_AutoReleasePool->release();
            return;
        }

        m_CommandQueue->signalDrawable(m_SurfaceRef);
        
        m_SurfaceRef->present();
        m_FrameNumber++;
        
        m_AutoReleasePool->release();
    }

    void MetalRenderer::Clean()
    {
        // Drain all in-flight GPU frames before destroying resources.
        for (uint32_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            dispatch_semaphore_wait(m_FrameSemaphore, DISPATCH_TIME_FOREVER);
        }
        
        // Restore count back to initial capacity (FRAME_OVERLAP) so GCD allows release.
        for (uint32_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            dispatch_semaphore_signal(m_FrameSemaphore);
        }
        
        // Safe to release frame semaphore now.
        dispatch_release(m_FrameSemaphore);
        m_FrameSemaphore = nullptr;
        
        m_MainDeletionQueue.Flush();

        // Delete offscreen render targets.
        m_ResourceManager->DeleteTexture(ShadowAtlasTexture);
        
        // Delete bind group layouts.
        m_ResourceManager->DeleteBindGroupLayout(m_EmptyBindingsLayout);
        m_ResourceManager->DeleteBindGroupLayout(m_DynamicBindingsLayout);

        // Delete bind groups.
        m_ResourceManager->DeleteBindGroup(m_EmptyBindings);
        
        // Delete render passes.
        for (auto& rp : m_RenderPasses)
        {
            m_ResourceManager->DeleteRenderPass(rp);
            rp = {};
        }
        for (auto& rp : m_ImGuiRenderPasses)
        {
            m_ResourceManager->DeleteRenderPass(rp);
            rp = {};
        }
        m_ResourceManager->DeleteRenderPass(m_RenderingRenderPass);
        m_RenderingRenderPass = {};
        
        m_ResourceManager->DeleteRenderPassLayout(m_RenderPassLayout);
        m_RenderPassLayout = {};
        
        // Delete surface depth textures.
        for (uint8_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            m_ResourceManager->DeleteTexture(m_DepthTextures[i]);
        }
        
        m_ResidencySet->release();
        
        TempUniformRingBuffer->Free();
        delete TempUniformRingBuffer;
        TempUniformRingBuffer = nullptr;

        m_ResourceManager->FlushAll();
    }

    void MetalRenderer::InvalidateGameViewTexture()
    {
        MetalTexture* gameViewTexture = m_ResourceManager->GetTexture(GameViewTexture);
        if (gameViewTexture != nullptr)
        {
            SetSceneViewAttachment((void*)gameViewTexture->Texture);
        }
    }

    void MetalRenderer::InvalidateSceneViewTexture()
    {
        MetalTexture* sceneViewTexture = m_ResourceManager->GetTexture(SceneViewTexture);
        if (sceneViewTexture != nullptr)
        {
            SetSceneViewAttachment((void*)sceneViewTexture->Texture);
        }
    }

    void MetalRenderer::Resize(int width, int height)
    {
        // Skip resize if width or height are 0.
        if (width == 0 || height == 0)
        {
            return;
        }
        
        // Update drawble surface size.
        m_Device->GetMetalLayer()->setDrawableSize(CGSizeMake(width, height));
        
        // Wait for device idle.
        for (uint32_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            dispatch_semaphore_wait(m_FrameSemaphore, DISPATCH_TIME_FOREVER);
        }
        
        // Cleanup old swapchain resources.
        for (auto& rp : m_RenderPasses)
        {
            m_ResourceManager->DeleteRenderPass(rp);
            rp = {};
        }
        for (auto& rp : m_ImGuiRenderPasses)
        {
            m_ResourceManager->DeleteRenderPass(rp);
            rp = {};
        }
        m_ResourceManager->DeleteRenderPass(m_RenderingRenderPass);
        m_RenderingRenderPass = {};
        
        m_ResourceManager->DeleteRenderPassLayout(m_RenderPassLayout);
        m_RenderPassLayout = {};
        
        // Recreate depth surface textures.
        for (uint8_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            m_ResourceManager->DeleteTexture(m_DepthTextures[i]);
            
            m_DepthTextures[i] = m_ResourceManager->CreateTexture({
                .debugName = "swapchain-depth-image",
                .dimensions = { width, height, 1 },
                .format = Format::D32_FLOAT,
                .internalFormat = Format::D32_FLOAT,
                .usage = TextureUsage::DEPTH_STENCIL,
                .aspect = TextureAspect::DEPTH,
                .createSampler = false,
            });
        }
        
        // Recreate swapchain and associated resources.
        CreateRenderPasses();

        // Call on resize callbacks.
        EventDispatcher::Get().Post(RendererResizedEvent(width, height));
        
        // Replenish tokens so frame rendering can resume cleanly.
        for (uint32_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            dispatch_semaphore_signal(m_FrameSemaphore);
        }
    }

    CommandBuffer* MetalRenderer::BeginCommandRecording(CommandBufferType type)
    {
        MTL4::CommandBuffer* cmd = nullptr;
        MetalCommandBuffer* mtlCmdObj = nullptr;
        
        switch (type)
        {
            case CommandBufferType::MAIN:
                cmd = GetCurrentFrame().MainCommandBuffer;
                mtlCmdObj = &m_MainCommandBuffers[m_FrameNumber.load() % FRAME_OVERLAP];
                break;
            case CommandBufferType::UI:
                cmd = GetCurrentFrame().ImGuiCommandBuffer;
                mtlCmdObj = &m_ImGuiCommandBuffers[m_FrameNumber.load() % FRAME_OVERLAP];
                break;
        }
        
        cmd->beginCommandBuffer(GetCurrentFrame().CommandAllocator);
        
        return mtlCmdObj;
    }

    void MetalRenderer::EnsureDrawableAcquired()
    {
        if (!m_SurfaceRef)
        {
            m_SurfaceRef = m_Device->GetMetalLayer()->nextDrawable();
            
            // Now set targets right before rendering
            MetalRenderPass* mainRp = m_ResourceManager->GetRenderPass(GetMainRenderPass());
            mainRp->SetColorTarget(0, m_SurfaceRef->texture());

            MetalRenderPass* imguiRp = m_ResourceManager->GetRenderPass(GetImGuiRenderPass());
            imguiRp->SetColorTarget(0, m_SurfaceRef->texture());
        }
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

    void MetalRenderer::CreateRenderPasses()
    {
        uint32_t frameIndex = 0;
        const auto& extents = m_Device->GetMetalLayer()->drawableSize();
        
        m_RenderPassLayout = m_ResourceManager->CreateRenderPassLayout({
            .debugName = "main-renderpass-layout",
            .depthTargetFormat = Format::D32_FLOAT,
            .subPasses = {
                { .depthTarget = true, .colorTargets = 1, },
            },
        });
        
        // Render passes for main rendering.
        for (auto& renderPassHandle : m_RenderPasses)
        {
            renderPassHandle = ResourceManager::Instance->CreateRenderPass({
                .debugName = "main-renderpass",
                .layout = m_RenderPassLayout,
                .depthTarget = {
                    .loadOp = LoadOperation::CLEAR,
                    .storeOp = StoreOperation::STORE,
                    .stencilLoadOp = LoadOperation::DONT_CARE,
                    .stencilStoreOp = StoreOperation::DONT_CARE,
                    .prevUsage = TextureLayout::UNDEFINED,
                    .nextUsage = TextureLayout::DEPTH_STENCIL_READ_ONLY,
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
        for (auto& renderPassHandle : m_ImGuiRenderPasses)
        {
            renderPassHandle = m_ResourceManager->CreateRenderPass({
                .debugName = "imgui-renderpass",
                .layout = m_RenderPassLayout,
                .depthTarget = {
                    .loadOp = LoadOperation::LOAD,
                    .storeOp = StoreOperation::STORE,
                    .stencilLoadOp = LoadOperation::DONT_CARE,
                    .stencilStoreOp = StoreOperation::DONT_CARE,
                    .prevUsage = TextureLayout::DEPTH_STENCIL_READ_ONLY,
                    .nextUsage = TextureLayout::DEPTH_STENCIL_READ_ONLY,
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
            .layout = m_RenderPassLayout,
            .depthTarget = {
                .loadOp = LoadOperation::CLEAR,
                .storeOp = StoreOperation::STORE,
                .stencilLoadOp = LoadOperation::DONT_CARE,
                .stencilStoreOp = StoreOperation::DONT_CARE,
                .prevUsage = TextureLayout::UNDEFINED,
                .nextUsage = TextureLayout::DEPTH_STENCIL_READ_ONLY,
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

    void MetalRenderer::BeginCapture()
    {
        // Check if Xcode or Instruments is attached and ready to receive a trace.
        if (m_CaptureManager->isCapturing())
        {
            std::cout << "A capture is already in progress." << std::endl;
            return;
        }

        // Configure the capture descriptor.
        m_CaptureDescriptor = MTL::CaptureDescriptor::alloc()->init();
        m_CaptureDescriptor->setCaptureObject(m_CommandQueue); // Capture scope tied to this queue
        m_CaptureDescriptor->setDestination(MTL::CaptureDestinationDeveloperTools); // Send to Xcode

        NS::Error* error = nullptr;
        
        // Start recording GPU commands.
        if (!m_CaptureManager->startCapture(m_CaptureDescriptor, &error))
        {
            std::cerr << "Failed to start capture: " << error->localizedDescription()->utf8String() << std::endl;
            m_CaptureDescriptor->release();
            return;
        }
    }

    void MetalRenderer::EndCapture()
    {
        if (m_CaptureManager->isCapturing())
        {
            m_CaptureManager->stopCapture();
            m_CaptureDescriptor->release();
        }
    }
}
