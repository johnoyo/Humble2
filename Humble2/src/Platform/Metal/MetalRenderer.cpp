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
        
        m_MainRenderFinishedSharedEvent = m_Device->Get()->newSharedEvent();
        m_MainRenderFinishedSharedEvent->setSignaledValue(0);
        
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
        
        // Global bindings for the 2D rendering.
        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            auto cameraBuffer2D = m_ResourceManager->CreateBuffer({
                .debugName = "camera-uniform-buffer",
                .usage = BufferUsage::UNIFORM,
                .usageHint = BufferUsageHint::DYNAMIC,
                .memoryUsage = MemoryUsage::GPU_CPU,
                .byteSize = 64,
                .initialData = nullptr,
            });

            m_MtlFrames[i].GlobalBindings2D = m_ResourceManager->CreateBindGroup({
                .debugName = "unlit-colored-bind-group",
                .layout = m_GlobalBindingsLayout2D,
                .buffers = {
                    { .buffer = cameraBuffer2D },
                }
            });
        }

        // Bindings for shadow rendering.
        uint64_t uniformOffset = Device::Instance->GetGPUProperties().limits.minUniformBufferOffsetAlignment;
        uint32_t alignedSize = UniformRingBuffer::CeilToNextMultiple(sizeof(glm::mat4), (uint32_t)uniformOffset);

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            auto lightSpaceBuffer = m_ResourceManager->CreateBuffer({
                .debugName = "light-space-buffer",
                .usage = BufferUsage::UNIFORM,
                .usageHint = BufferUsageHint::DYNAMIC,
                .memoryUsage = MemoryUsage::GPU_CPU,
                .byteSize = 16 * alignedSize,
                .initialData = nullptr
            });

            m_MtlFrames[i].ShadowBindings = m_ResourceManager->CreateBindGroup({
                .debugName = "shadow-bind-group",
                .layout = m_ShadowBindingsLayout,
                .buffers = {
                    { .buffer = lightSpaceBuffer, .range = 64 }, // TODO: Investigate if '.range' should be alignedSize!
                }
            });
        }

        // Bindings for debug rendering.
        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            auto cameraBuffer = m_ResourceManager->CreateBuffer({
                .debugName = "debug-draw-camera-uniform-buffer",
                .usage = BufferUsage::UNIFORM,
                .usageHint = BufferUsageHint::DYNAMIC,
                .memoryUsage = MemoryUsage::GPU_CPU,
                .byteSize = sizeof(CameraData),
                .initialData = nullptr,
            });

            m_MtlFrames[i].DebugBindings = m_ResourceManager->CreateBindGroup({
                .debugName = "debug-draw-bind-group",
                .layout = m_GlobalBindingsLayout2D,
                .buffers = { { .buffer = cameraBuffer } }
            });
        }
    }

    void MetalRenderer::PostInitialize()
    {
        // Global bindings for the 3D rendering.
        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            auto cameraBuffer3D = m_ResourceManager->CreateBuffer({
                .debugName = "camera-uniform-buffer",
                .usage = BufferUsage::UNIFORM,
                .usageHint = BufferUsageHint::DYNAMIC,
                .memoryUsage = MemoryUsage::GPU_CPU,
                .byteSize = sizeof(CameraData),
                .initialData = nullptr,
            });

            auto lightBuffer = m_ResourceManager->CreateBuffer({
                .debugName = "light-uniform-buffer",
                .usage = BufferUsage::UNIFORM,
                .usageHint = BufferUsageHint::DYNAMIC,
                .memoryUsage = MemoryUsage::GPU_CPU,
                .byteSize = sizeof(LightData),
                .initialData = nullptr,
            });

            m_MtlFrames[i].GlobalBindings3D = m_ResourceManager->CreateBindGroup({
                .debugName = "global-bind-group",
                .layout = m_GlobalBindingsLayout3D,
                .textures = { { ShadowAtlasTexture } },
                .buffers = {
                    { .buffer = cameraBuffer3D },
                    { .buffer = lightBuffer },
                }
            });
        }

        // Global bindings for presenting.
        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            m_MtlFrames[i].GlobalPresentBindings = m_ResourceManager->CreateBindGroup({
                .debugName = "global-present-bind-group",
                .layout = GetGlobalPresentBindingsLayout(),
                .textures = { { MainColorTexture, TextureLayout::SHADER_READ_ONLY } },
            });
        }
        
        // Register event to update the drawble size on resize.
        EventDispatcher::Get().Register<FramebufferSizeEvent>([this](const FramebufferSizeEvent& e)
        {
            Resize(e.Width, e.Height);
        });
    }

    void MetalRenderer::BeginFrame()
    {
        m_AutoReleasePool = NS::AutoreleasePool::alloc()->init();
        
        // Signal that frame is ready.
        if (m_FrameNumber.load() >= FRAME_OVERLAP)
        {
            m_FrameAvailableSharedEvent->waitUntilSignaledValue(m_FrameNumber.load() - FRAME_OVERLAP, 1000);
        }
        
        // Flush any pending deletions that occured in the frames before the current one.
        m_ResourceManager->Flush(m_FrameNumber.load());
        
        // Reset frames command allocator.
        GetCurrentFrame().CommandAllocator->reset();
        
        // Attach surface color target texture to main and imgui render passes.
        m_SurfaceRef = m_Device->GetMetalLayer()->nextDrawable();
        
        MetalRenderPass* mainRp = m_ResourceManager->GetRenderPass(GetMainRenderPass());
        mainRp->SetColorTarget(0, m_SurfaceRef->texture());
        
        MetalRenderPass* imguiRp = m_ResourceManager->GetRenderPass(GetImGuiRenderPass());
        imguiRp->SetColorTarget(0, m_SurfaceRef->texture());
        
        SwapAndResetStats();
    }

    void MetalRenderer::EndFrame()
    {
        m_CommandQueue->signalDrawable(m_SurfaceRef);
    }

    void MetalRenderer::Present()
    {
        m_SurfaceRef->present();
        
        m_CommandQueue->signalEvent(m_FrameAvailableSharedEvent, m_FrameNumber.load());
        m_FrameNumber++;
        
        m_AutoReleasePool->release();
    }

    void MetalRenderer::Clean()
    {
        m_MainDeletionQueue.Flush();

        // Delete offscreen render targets.
        m_ResourceManager->DeleteTexture(IntermediateColorTexture);
        m_ResourceManager->DeleteTexture(MainColorTexture);
        m_ResourceManager->DeleteTexture(MainDepthTexture);
        m_ResourceManager->DeleteTexture(ShadowAtlasTexture);
        
        // Delete bind group layouts.
        m_ResourceManager->DeleteBindGroupLayout(m_ShadowBindingsLayout);
        m_ResourceManager->DeleteBindGroupLayout(m_GlobalBindingsLayout2D);
        m_ResourceManager->DeleteBindGroupLayout(m_GlobalBindingsLayout3D);
        m_ResourceManager->DeleteBindGroupLayout(m_GlobalPresentBindingsLayout);
        m_ResourceManager->DeleteBindGroupLayout(m_EmptyBindingsLayout);
        m_ResourceManager->DeleteBindGroupLayout(m_DynamicBindingsLayout);

        // Delete bind groups.
        m_ResourceManager->DeleteBindGroup(m_EmptyBindings);

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            MetalBindGroupCold* shadowBindGroupCold = m_ResourceManager->GetBindGroupCold(m_MtlFrames[i].ShadowBindings);

            for (auto& bufferEntry : shadowBindGroupCold->Buffers)
            {
                m_ResourceManager->DeleteBuffer(bufferEntry.buffer);
            }

            m_ResourceManager->DeleteBindGroup(m_MtlFrames[i].ShadowBindings);
            m_ResourceManager->DeleteBindGroup(m_MtlFrames[i].GlobalBindings2D);
            m_ResourceManager->DeleteBindGroup(m_MtlFrames[i].GlobalBindings3D);
            m_ResourceManager->DeleteBindGroup(m_MtlFrames[i].GlobalPresentBindings);
            m_ResourceManager->DeleteBindGroup(m_MtlFrames[i].DebugBindings);
        }
        
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
        if (m_FrameNumber.load() > 0)
        {
            m_FrameAvailableSharedEvent->waitUntilSignaledValue(m_FrameNumber.load() - 1, UINT64_MAX);
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
        
        // Destroy old offscreen textures.
        m_ResourceManager->DeleteTexture(IntermediateColorTexture);
        m_ResourceManager->DeleteTexture(MainColorTexture);
        m_ResourceManager->DeleteTexture(MainDepthTexture);
        
        // Recreate the offscreen textures (render targets).
        IntermediateColorTexture = ResourceManager::Instance->CreateTexture({
            .debugName = "intermediate-color-target",
            .dimensions = { width, height, 1 },
            .format = Format::RGBA16_FLOAT,
            .internalFormat = Format::RGBA16_FLOAT,
            .usage = { TextureUsage::RENDER_ATTACHMENT, TextureUsage::SAMPLED },
            .aspect = TextureAspect::COLOR,
            .sampler =
            {
                .filter = TextureFilter::LINEAR,
                .wrap = Wrap::CLAMP_TO_EDGE,
            }
        });

        MainColorTexture = ResourceManager::Instance->CreateTexture({
            .debugName = "viewport-color-target",
            .dimensions = { width, height, 1 },
            .format = Format::BGRA8_UNORM,
            .internalFormat = Format::BGRA8_UNORM,
            .usage = { TextureUsage::RENDER_ATTACHMENT, TextureUsage::SAMPLED },
            .aspect = TextureAspect::COLOR,
            .sampler =
            {
                .filter = TextureFilter::LINEAR,
                .wrap = Wrap::CLAMP_TO_EDGE,
            }
        });

        MainDepthTexture = ResourceManager::Instance->CreateTexture({
            .debugName = "viewport-depth-target",
            .dimensions = { width, height, 1 },
            .format = Format::D32_FLOAT,
            .internalFormat = Format::D32_FLOAT,
            .usage = TextureUsage::DEPTH_STENCIL,
            .aspect = TextureAspect::DEPTH,
            .createSampler = true,
            .sampler =
            {
                .filter = TextureFilter::NEAREST,
                .wrap = Wrap::CLAMP_TO_EDGE,
            }
        });
        
        // Update descriptor sets (for the full screen quad shader).
        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            m_ResourceManager->DeleteBindGroup(m_MtlFrames[i].GlobalPresentBindings);

            m_MtlFrames[i].GlobalPresentBindings = m_ResourceManager->CreateBindGroup({
                .debugName = "global-present-bind-group",
                .layout = Renderer::Instance->GetGlobalPresentBindingsLayout(),
                .textures = { { MainColorTexture, TextureLayout::SHADER_READ_ONLY } },  // Updated with new size
            });
        }
        
        // Update viewport texture attachment (used in imgui).
        MetalTexture* viewportTexture = m_ResourceManager->GetTexture(MainColorTexture);
        SetViewportAttachment((void*)viewportTexture->Texture);
        
        // Call on resize callbacks.
        for (auto&& [name, callback] : m_OnResizeCallbacks)
        {
            callback(width, height);
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

    void MetalRenderer::SignalMainRenderFinishedEvent()
    {
        m_CommandQueue->signalEvent(m_MainRenderFinishedSharedEvent, m_FrameNumber.load());
    }

    void MetalRenderer::WaitForMainRenderFinishedEvent()
    {
        m_CommandQueue->wait(m_MainRenderFinishedSharedEvent, m_FrameNumber.load());
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
            .layout = m_RenderPassLayout,
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
