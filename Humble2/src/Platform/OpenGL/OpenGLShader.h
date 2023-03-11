#pragma once

#include "../Humble2/Renderer/Shader.h"
#include "../../Humble2/Utilities/Log.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <emscripten\emscripten.h>
	#include <GLFW/glfw3.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

namespace HBL
{
	class OpenGLShader final : public Shader
	{
	public:
		OpenGLShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);

		virtual void Bind() override;
		virtual void UnBind() override;
	private:
		uint32_t Compile(uint32_t type, const std::string& source);
		void Attach();
	};
}