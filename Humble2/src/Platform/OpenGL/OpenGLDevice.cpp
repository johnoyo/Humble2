#include "OpenGLDevice.h"

namespace HBL2
{
	void OpenGLDevice::Initialize()
	{
		int32_t uniformBufferAlignSize;
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformBufferAlignSize);
		m_GPUProperties.limits.minUniformBufferOffsetAlignment = uniformBufferAlignSize;

		const GLubyte* vendor = glGetString(GL_VENDOR);
		const GLubyte* renderer = glGetString(GL_RENDERER);
		const GLubyte* version = glGetString(GL_VERSION);

		m_GPUProperties.vendorID = (const char*)vendor;
		m_GPUProperties.deviceName = (const char*)renderer;
		m_GPUProperties.driverVersion = (const char*)version;

		HBL2_CORE_INFO("OpenGL Info:\n\
				Vendor: {},\n\
				Renderer: {},\n\
				Version: {}", m_GPUProperties.vendorID, m_GPUProperties.deviceName, m_GPUProperties.driverVersion);

		HBL2_CORE_INFO("GPU Limits: \n\
				The GPU has a minimum buffer alignment of {}", m_GPUProperties.limits.minUniformBufferOffsetAlignment);
	}

	void OpenGLDevice::Destroy()
	{
	}
}
