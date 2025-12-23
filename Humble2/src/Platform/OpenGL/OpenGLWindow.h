#pragma once

#define GLFW_INCLUDE_NONE
#include <GL/glew.h>

#include "Core/Window.h"

namespace HBL2
{
	class OpenGLWindow final : public Window
	{
		~OpenGLWindow() = default;

		virtual void Create() override;
		virtual void Setup() override;
	};
}