#include "OpenGLRendererAPI.h"

namespace HBL2
{
	OpenGLRendererAPI::OpenGLRendererAPI()
	{
#ifdef DEBUG
		GLDebug::EnableGLDebugging();
#endif
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBlendEquation(GL_FUNC_ADD);
	}

	void OpenGLRendererAPI::SetViewport(GLint x, GLint y, GLsizei width, GLsizei height)
	{
		glViewport(x, y, width, height);
	}

	void OpenGLRendererAPI::ClearScreen(glm::vec4 color)
	{
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(color.r, color.g, color.b, color.a);
	}

	void OpenGLRendererAPI::Submit(VertexBuffer* vertexBuffer)
	{
		glDrawElements(GL_TRIANGLES, (vertexBuffer->BatchSize / 4) * 6, GL_UNSIGNED_INT, NULL);
	}
}


