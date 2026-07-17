#pragma once

#include "Base.h"
#include "Core/Context.h"

#include "Scene/Components.h"
#include "Renderer/Renderer.h"

#include "MetalDevice.h"
#include "MetalCommandBuffer.h"

#include "MetalCommon.h"

#include "Utilities/Collections/DeletionQueue.h"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

namespace HBL2
{
    constexpr unsigned int FRAME_OVERLAP = 2;

    struct MtlFrameData
    {
        MTL4::CommandBuffer* MainCommandBuffer = nullptr;
        MTL4::CommandBuffer* ImGuiCommandBuffer = nullptr;
        MTL4::CommandAllocator* CommandAllocator = nullptr;
        MTL4::ArgumentTable* GlobalArgumentTable = nullptr;
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
        
        virtual void* GetDepthAttachment() override { return nullptr; }
        virtual void* GetColorAttachment() override;
        
        virtual void SetViewportAttachment(void* viewportTextureRef) override {}
        virtual void* GetViewportAttachment() override { return nullptr; }

        virtual Handle<BindGroup> GetShadowBindings() override { return {}; }
        virtual Handle<BindGroup> GetGlobalBindings2D() override { return {}; }
        virtual Handle<BindGroup> GetGlobalBindings3D() override { return {}; }
        virtual Handle<BindGroup> GetGlobalPresentBindings() override { return {}; }
        virtual Handle<BindGroup> GetDebugBindings() override { return {}; }

        virtual Handle<RenderPass> GetMainRenderPass() override { return {}; }
        virtual Handle<RenderPass> GetImGuiRenderPass() override { return {}; }
        virtual Handle<RenderPass> GetRenderingRenderPass() override { return {}; }
        
        virtual const uint32_t GetFrameIndex() const override { return m_FrameNumber.load() % FRAME_OVERLAP; }
        
        MtlFrameData& GetCurrentFrame() { return m_MtlFrames[m_FrameNumber.load() % FRAME_OVERLAP]; }
        const MtlFrameData& GetCurrentFrame() const { return m_MtlFrames[m_FrameNumber.load() % FRAME_OVERLAP]; }
        
        MTL4::CommandQueue* GetCommandQueue() { return m_CommandQueue; }
        const MTL4::CommandQueue* GetCommandQueue() const { return m_CommandQueue; }
        
    protected:
        virtual void PreInitialize() override;
        virtual void PostInitialize() override;
        
    private:
        
        
    private:
        MetalDevice* m_Device = nullptr;
        MTL4::CommandQueue* m_CommandQueue = nullptr;
        std::array<MtlFrameData, FRAME_OVERLAP> m_MtlFrames;
        MTL::ResidencySet* m_ResidencySet = nullptr;
        std::array<MTL::Texture*, FRAME_OVERLAP> m_DepthTextures;
        MTL::SharedEvent* m_FrameAvailableSharedEvent = nullptr;
        
        std::array<MetalCommandBuffer, FRAME_OVERLAP> m_MainCommandBuffers;
        std::array<MetalCommandBuffer, FRAME_OVERLAP> m_ImGuiCommandBuffers;
    };
}
