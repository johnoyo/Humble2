#pragma once

#include "Base.h"
#include "Renderer/Device.h"
#include "Platform/Metal/MetalWindow.h"

#include "MetalCommon.h"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

namespace HBL2
{
    static constexpr auto MAX_FRAMES_IN_FLIGHT = 3u;
    static constexpr auto PIXEL_FORMAT = MTL::PixelFormatBGRA8Unorm_sRGB;

    class MetalDevice : public Device
    {
    public:
        ~MetalDevice() = default;
        
        virtual void Initialize() override;
        virtual void Destroy() override;
        virtual bool HasContext() override { return true; }
        virtual void SetContext(ContextType ctxType) override {}
        
        MTL::Device* Get() { return m_Device; }
        const MTL::Device* Get() const { return m_Device; }
        
        CA::MetalLayer* GetMetalLayer() { return m_MetalLayer; }
        const CA::MetalLayer* GetMetalLayer() const { return m_MetalLayer; }
        
    private:
        MTL::Device* m_Device = nullptr;
        CA::MetalLayer* m_MetalLayer = nullptr;
    };
}
