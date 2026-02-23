#include "OpenGLDevice.h"

#include "Core\Window.h"

#include <GLFW/glfw3.h>
#include <span>

namespace HBL2
{
	void OpenGLDevice::Initialize()
	{
		Window::Instance->Setup();

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

	void OpenGLDevice::SetContext(ContextType ctxType)
	{
		static thread_local int localWorkerIndex = -1;

		if (ctxType == ContextType::FLUSH_CLEAR)
		{
			GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
			glFlush();
			glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
			glDeleteSync(fence);

			glfwMakeContextCurrent(nullptr);

			return;
		}

		if (ctxType == ContextType::CLEAR)
		{
			glFlush();
			glfwMakeContextCurrent(nullptr);

			return;
		}

		auto windowContexts = Window::Instance->GetWorkerHandles();

		// Assign a unique context index per thread once
		if (localWorkerIndex == -1)
		{
			int index = m_WorkerIndex.fetch_add(1, std::memory_order_relaxed);
			if (index >= static_cast<int>(windowContexts.size()))
			{
				// no available contexts
				m_WorkerIndex.fetch_sub(1, std::memory_order_relaxed);
				return;
			}
			localWorkerIndex = index;
		}

		glfwMakeContextCurrent(windowContexts[localWorkerIndex]);
	}
}
