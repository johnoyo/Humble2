#pragma once

#include "Renderer\Rewrite\Device.h"

namespace HBL2
{
	class OpenGLDevice : public Device
	{
	public:
		~OpenGLDevice() = default;

		virtual void Initialize() override;
		virtual void Destroy() override;
	};
}