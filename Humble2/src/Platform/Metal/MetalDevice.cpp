#include "MetalDevice.h"

#include "MetalUtils.h"
#include "Renderer/Renderer.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace HBL2
{
    void MetalDevice::Initialize()
    {
        Window::Instance->Setup();
        
        m_Device = MTL::CreateSystemDefaultDevice();
        m_MetalLayer = CA::MetalLayer::layer();
        {
            m_MetalLayer->setDevice(m_Device);
            m_MetalLayer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
            m_MetalLayer->setMaximumDrawableCount(FRAME_OVERLAP);
            m_MetalLayer->setFramebufferOnly(true);
            m_MetalLayer->setDisplaySyncEnabled(Window::Instance->GetSpec().VerticalSync);
        }
        
        ConnectWindowWithMetal(Window::Instance->GetHandle(), m_MetalLayer);
        
        // Set GPU properties
        m_GPUProperties.limits.minUniformBufferOffsetAlignment = 32;

        HBL2_CORE_INFO("GPU Limits: \n\
                The GPU has a minimum buffer alignment of {}", m_GPUProperties.limits.minUniformBufferOffsetAlignment);
        
        // Set surface size from window.
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(Window::Instance->GetHandle(), &fbWidth, &fbHeight);
        
        if (fbWidth == 0 || fbHeight == 0)
        {
            return;
        }
        
        m_MetalLayer->setDrawableSize(CGSizeMake(fbWidth, fbHeight));
    }

    void MetalDevice::Destroy()
    {
        m_MetalLayer->release();
        m_Device->release();
    }
}
