#pragma once

#include "Base.h"
#include "Core/Context.h"

#include "Scene/Components.h"
#include "Renderer/Renderer.h"

#include "MetalCommandBuffer.h"

#include "MetalCommon.h"

#include "Utilities/Collections/DeletionQueue.h"

namespace HBL2
{
    constexpr unsigned int FRAME_OVERLAP = 2;

    struct MtlFrameData
    {
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
        
        virtual void SetViewportAttachment(Handle<Texture> viewportTexture) override {}
        virtual void* GetViewportAttachment() override { return nullptr; }

        virtual Handle<BindGroup> GetShadowBindings() override { return {}; }
        virtual Handle<BindGroup> GetGlobalBindings2D() override { return {}; }
        virtual Handle<BindGroup> GetGlobalBindings3D() override { return {}; }
        virtual Handle<BindGroup> GetGlobalPresentBindings() override { return {}; }
        virtual Handle<BindGroup> GetDebugBindings() override { return {}; }

        virtual Handle<FrameBuffer> GetMainFrameBuffer() override { return {}; }
        virtual const uint32_t GetFrameIndex() const override { return m_FrameNumber.load() % FRAME_OVERLAP; }
        
        const MtlFrameData& GetCurrentFrame() const { return m_MtlFrames[m_FrameNumber.load() % FRAME_OVERLAP]; }
        
    protected:
        virtual void PreInitialize() override;
        virtual void PostInitialize() override;
        
    private:
        
        
    private:
        MtlFrameData m_MtlFrames[FRAME_OVERLAP];
    };
}
