#pragma once

#include "Core/Window.h"

namespace HBL2
{
	class OpenGLWindow final : public Window
	{
		~OpenGLWindow() = default;

		virtual void Create() override;
		virtual void Present() override;
	};
}