#pragma once

#include "../Base.h"

#define GLFW_INCLUDE_NONE
#include <GL/glew.h>

namespace HBL2
{
	namespace GLDebug
	{
		enum class DebugLogLevel
		{
			None = 0, HighAssert = 1, High = 2, Medium = 3, Low = 4, Notification = 5
		};

		void EnableGLDebugging();
		void SetGLDebugLogLevel(DebugLogLevel level);
		void OpenGLLogMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
	}
}