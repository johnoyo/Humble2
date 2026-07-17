#include "MetalDevice.h"

#include "MetalUtils.h"

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
            m_MetalLayer->setPixelFormat(PIXEL_FORMAT);
            m_MetalLayer->setMaximumDrawableCount(MAX_FRAMES_IN_FLIGHT);
            m_MetalLayer->setFramebufferOnly(true);
        }
        
        ConnectWindowWithMetal(Window::Instance->GetHandle(), m_MetalLayer);
    }

    void MetalDevice::Destroy()
    {
        m_MetalLayer->release();
        m_Device->release();
    }
}
