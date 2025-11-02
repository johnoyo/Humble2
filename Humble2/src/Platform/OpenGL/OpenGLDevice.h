#pragma once

#include "Base.h"
#include "Renderer\Device.h"

#include <GL\glew.h>

#include <cstdint>

namespace HBL2
{
	class OpenGLDevice : public Device
	{
	public:
		~OpenGLDevice() = default;

		virtual void Initialize() override;
		virtual void Destroy() override;
		virtual void SetContext(ContextType ctxType);

	private:
		std::atomic<int32_t> m_WorkerIndex;
	};
}