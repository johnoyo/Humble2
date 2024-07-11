#pragma once

#include "Base.h"
#include "Renderer/Shader.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

namespace HBL2
{
	class OpenGLShader final : public Shader
	{
	public:
		OpenGLShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource);
		virtual ~OpenGLShader() {}

		virtual void Bind() override;
		virtual void UnBind() override;

		virtual void SetFloat1(const float value, const std::string& uniformName) override;
		virtual void SetFloat2(const glm::vec2& vec2, const std::string& uniformName) override;
		virtual void SetMat4(const glm::mat4& mat4, const std::string& uniformName) override;
		virtual void SetInt1(const int32_t value, const std::string& uniformName) override;
		virtual void SetIntPtr1(const GLint* value, GLsizei count, const std::string& uniformName) override;
	private:
		uint32_t Compile(uint32_t type, const std::string& source);
		void Attach();
	};
}