#pragma once

#include "Base.h"
#include "Core/Context.h"

#include "Scene/Components.h"
#include "Renderer/Renderer.h"

#include "Platform/Metal/MetalDevice.h"
#include "Platform/Metal/MetalCommandBuffer.h"

#include "Platform/Metal/MetalCommon.h"

#include "Utilities/Collections/DeletionQueue.h"

namespace HBL2
{
    static constexpr uint32_t VERTEX_BUFFER_BINDING_IDX = 15;

    class MetalResourceManager;

    struct MtlFrameData
    {
        MTL4::CommandBuffer* MainCommandBuffer = nullptr;
        MTL4::CommandBuffer* ImGuiCommandBuffer = nullptr;
        MTL4::CommandAllocator* CommandAllocator = nullptr;
        MTL4::ArgumentTable* GlobalArgumentTable = nullptr;
        
        Handle<BindGroup> ShadowBindings;
        Handle<BindGroup> DebugBindings;
        Handle<BindGroup> GlobalBindings2D;
        Handle<BindGroup> GlobalBindings3D;
        Handle<BindGroup> GlobalPresentBindings;
    };

    struct MtlUploadContext
    {
        MTL4::CommandAllocator* Allocator = nullptr;
        MTL4::CommandBuffer* CommandBuffer = nullptr;
        MTL::SharedEvent* Event = nullptr;
        uint64_t EventValue = 0;
    };

    class MetalRenderer final : public Renderer
    {
    public:
        virtual ~MetalRenderer() = default;
        
        virtual void BeginFrame() override;
        virtual void EndFrame() override;
        virtual void Present() override;
        virtual void Clean() override;
        
        virtual CommandBuffer* BeginCommandRecording(CommandBufferType type) override;
        
        virtual void SetViewportAttachment(void* viewportTextureRef) override { m_ColorAttachmentID = (MTL::Texture*)viewportTextureRef; }
        virtual void* GetViewportAttachment() override { return m_ColorAttachmentID; }

        virtual Handle<BindGroup> GetShadowBindings() override { return m_MtlFrames[m_FrameNumber.load() % FRAME_OVERLAP].ShadowBindings; }
        virtual Handle<BindGroup> GetGlobalBindings2D() override { return m_MtlFrames[m_FrameNumber.load() % FRAME_OVERLAP].GlobalBindings2D; }
        virtual Handle<BindGroup> GetGlobalBindings3D() override { return m_MtlFrames[m_FrameNumber.load() % FRAME_OVERLAP].GlobalBindings3D; }
        virtual Handle<BindGroup> GetGlobalPresentBindings() override { return m_MtlFrames[m_FrameNumber.load() % FRAME_OVERLAP].GlobalPresentBindings; }
        virtual Handle<BindGroup> GetDebugBindings() override { return m_MtlFrames[m_FrameNumber.load() % FRAME_OVERLAP].DebugBindings; }

        virtual Handle<RenderPass> GetMainRenderPass() override { return m_RenderPasses[m_FrameNumber.load() % FRAME_OVERLAP]; }
        virtual Handle<RenderPass> GetImGuiRenderPass() override { return m_ImGuiRenderPasses[m_FrameNumber.load() % FRAME_OVERLAP]; }
        virtual Handle<RenderPass> GetRenderingRenderPass() override { return m_RenderingRenderPass; }
        
        virtual const uint32_t GetFrameIndex() const override { return m_FrameNumber.load() % FRAME_OVERLAP; }
        
        MtlFrameData& GetCurrentFrame() { return m_MtlFrames[m_FrameNumber.load() % FRAME_OVERLAP]; }
        const MtlFrameData& GetCurrentFrame() const { return m_MtlFrames[m_FrameNumber.load() % FRAME_OVERLAP]; }
        
        MTL4::CommandQueue* GetCommandQueue() { return m_CommandQueue; }
        const MTL4::CommandQueue* GetCommandQueue() const { return m_CommandQueue; }
        
        CA::MetalDrawable* GetCurrentSurface() { return m_SurfaceRef; }
        const CA::MetalDrawable* GetCurrentSurface() const { return m_SurfaceRef; }
        
        void MakeResident(std::initializer_list<MTL::Allocation*> resources);
        void RemoveResident(MTL::Allocation* resource);
        void ImmediateSubmit(const std::function<void(MTL4::ComputeCommandEncoder*)>& fn);
        
    protected:
        virtual void PreInitialize() override;
        virtual void PostInitialize() override;
        
    private:
        void CreateUploadContextCommands();
        MTL::Texture* CreateDepthTexture(uint32_t width, uint32_t height, uint32_t frameIdx);
        void CreateRenderPasses();
        void Resize(int width, int height);
        
    private:
        MetalDevice* m_Device = nullptr;
        MetalResourceManager* m_ResourceManager;
        DeletionQueue m_MainDeletionQueue;
        std::mutex m_DeletionQueueMutex;
        
        CA::MetalDrawable* m_SurfaceRef = nullptr;
        MTL4::CommandQueue* m_CommandQueue = nullptr;
        std::array<MtlFrameData, FRAME_OVERLAP> m_MtlFrames;
        MTL::ResidencySet* m_ResidencySet = nullptr;
        std::array<Handle<Texture>, FRAME_OVERLAP> m_DepthTextures;
        MTL::SharedEvent* m_FrameAvailableSharedEvent = nullptr;
        
        std::array<MetalCommandBuffer, FRAME_OVERLAP> m_MainCommandBuffers;
        std::array<MetalCommandBuffer, FRAME_OVERLAP> m_ImGuiCommandBuffers;
        
        std::mutex m_ResidencyMutex;
        static thread_local MtlUploadContext s_UploadContext;
                
        std::array<Handle<RenderPass>, FRAME_OVERLAP> m_RenderPasses;
        std::array<Handle<RenderPass>, FRAME_OVERLAP> m_ImGuiRenderPasses;
        Handle<RenderPass> m_RenderingRenderPass;
        Handle<RenderPassLayout> m_RenderPassLayout;
        
        MTL::Texture* m_ColorAttachmentID = nullptr;
        
        bool m_ResizeRequested = false;
        int m_Width = 0;
        int m_Height = 0;
    };
}
