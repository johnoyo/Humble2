#include "OpenGLRendererAPI.h"

namespace HBL
{
	void OpenGLRendererAPI::Initialize()
	{
	}

	void OpenGLRendererAPI::DrawQuad()
	{
	}

	void OpenGLRendererAPI::SetViewport(GLint x, GLint y, GLsizei width, GLsizei height)
	{
		glViewport(x, y, width, height);
	}
}


