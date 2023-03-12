#include "OpenGLShader.h"

namespace HBL
{
	OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertexSource, const std::string& fragmentSource)
		: Shader(name, vertexSource, fragmentSource)
	{
		Shader::Add(m_Name, this);
		Attach();
	}

	uint32_t OpenGLShader::Compile(uint32_t type, const std::string& source)
	{
		uint32_t id = glCreateShader(type);
		const char* src = source.c_str();
		glShaderSource(id, 1, &src, nullptr);
		glCompileShader(id);

		int result;
		glGetShaderiv(id, GL_COMPILE_STATUS, &result);
		if (result == GL_FALSE)
		{
			int lenght;
			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &lenght);
			char* message = (char*)alloca(lenght * sizeof(char));
			glGetShaderInfoLog(id, lenght, &lenght, message);
			HBL_CORE_ERROR("Failed to compile {0} shader.", (type == GL_VERTEX_SHADER ? "vertex" : "fragment"));
			HBL_CORE_ERROR(message);
			glDeleteShader(id);

			return 0;
		}

		return id;
	}

	void OpenGLShader::Attach()
	{
		unsigned int program = glCreateProgram();
		unsigned int vs = Compile(GL_VERTEX_SHADER, m_VertexSource);
		unsigned int fs = Compile(GL_FRAGMENT_SHADER, m_FragmentSource);

		glAttachShader(program, vs);
		glAttachShader(program, fs);
		glLinkProgram(program);
		glValidateProgram(program);

		glDeleteShader(vs);
		glDeleteShader(fs);

		m_ID = program;
	}

	void OpenGLShader::Bind()
	{
		glUseProgram(m_ID);
	}

	void OpenGLShader::UnBind()
	{
		glUseProgram(0);
	}

	void OpenGLShader::SetMat4(const glm::mat4& mat4, const std::string& uniformName)
	{
		auto location1 = glGetUniformLocation(m_ID, uniformName.c_str());
		if (location1 == -1)
			HBL_CORE_ERROR("Uniform: {0} not found in shader with name: {1}.", uniformName, m_Name);

		glUniformMatrix4fv(location1, 1, GL_FALSE, glm::value_ptr(mat4));
	}

	void OpenGLShader::SetInt1(const int32_t value, const std::string& uniformName)
	{
		auto location = glGetUniformLocation(m_ID, uniformName.c_str());
		if (location == -1)
			HBL_CORE_ERROR("Uniform: {0} not found in shader with name: {1}.", uniformName, m_Name);

		glUniform1i(location, value);
	}

	void OpenGLShader::SetIntPtr1(const GLint* value, GLsizei count, const std::string& uniformName)
	{
		auto location = glGetUniformLocation(m_ID, uniformName.c_str());
		if (location == -1)
			HBL_CORE_ERROR("Uniform: {0} not found in shader with name: {1}.", uniformName, m_Name);

		glUniform1iv(location, count, value);
	}
}