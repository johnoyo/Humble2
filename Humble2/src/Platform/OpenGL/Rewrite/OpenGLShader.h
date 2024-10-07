#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#ifdef EMSCRIPTEN
	#define GLFW_INCLUDE_ES3
	#include <GLFW/glfw3.h>
#else
	#define GLFW_INCLUDE_NONE
	#include <GL/glew.h>
#endif

#include "Utilities\Span.h"

#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>

namespace HBL2
{
	struct OpenGLShader
	{
		OpenGLShader() = default;
		OpenGLShader(const ShaderDescriptor&& desc)
		{
			DebugName = desc.debugName;

			Program = glCreateProgram();
			GLuint vs = Compile(GL_VERTEX_SHADER, desc.VS.entryPoint, desc.VS.code);
			GLuint fs = Compile(GL_FRAGMENT_SHADER, desc.FS.entryPoint, desc.FS.code);

			glAttachShader(Program, vs);
			glAttachShader(Program, fs);
			glLinkProgram(Program);
			glValidateProgram(Program);

			GLint linkStatus;
			glGetProgramiv(Program, GL_LINK_STATUS, &linkStatus);
			if (linkStatus == GL_FALSE)
			{
				GLint logLength = 0;
				glGetProgramiv(Program, GL_INFO_LOG_LENGTH, &logLength);
				std::vector<GLchar> log(logLength);
				glGetProgramInfoLog(Program, logLength, &logLength, log.data());
				HBL2_CORE_ERROR("Shader Program linking failed!");
			}

			glDeleteShader(vs);
			glDeleteShader(fs);

			glGenVertexArrays(1, &RenderPipeline);
			glBindVertexArray(RenderPipeline);

			VertexBufferBindings = desc.renderPipeline.vertexBufferBindings;
		}

		GLuint Compile(GLuint type, const char* entryPoint, const std::vector<uint32_t>& binaryCode)
		{
			GLuint shaderID = glCreateShader(type);
			glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, binaryCode.data(), (GLuint)binaryCode.size() * sizeof(uint32_t));
			glSpecializeShaderARB(shaderID, entryPoint, 0, nullptr, nullptr);

			GLint compileStatus;
			glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);
			if (compileStatus == GL_FALSE)
			{
				GLint logLength = 0;
				glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);
				std::vector<GLchar> log(logLength);
				glGetShaderInfoLog(shaderID, logLength, &logLength, log.data());
				HBL2_CORE_ERROR("Shader compilation failed!");
			}

			return shaderID;
		}

		const char* DebugName = "";
		GLuint Program = 0;
		GLuint RenderPipeline = 0;
		std::vector<ShaderDescriptor::RenderPipeline::VertexBufferBinding> VertexBufferBindings;
	};
}