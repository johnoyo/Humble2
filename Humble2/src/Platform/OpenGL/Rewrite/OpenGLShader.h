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

#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>

namespace HBL2
{
	struct OpenGLShader
	{
		OpenGLShader() = default;
		OpenGLShader(const ShaderDescriptor& desc)
		{
			DebugName = desc.debugName;

			Program = glCreateProgram();
			uint32_t vs = Compile(GL_VERTEX_SHADER, desc.VS.entryPoint, desc.VS.code);
			uint32_t fs = Compile(GL_FRAGMENT_SHADER, desc.FS.entryPoint, desc.FS.code);

			glAttachShader(Program, vs);
			glAttachShader(Program, fs);
			glLinkProgram(Program);
			glValidateProgram(Program);

			glDeleteShader(vs);
			glDeleteShader(fs);

			glGenVertexArrays(1, &RenderPipeline);
			glBindVertexArray(RenderPipeline);
		}

		uint32_t Compile(uint32_t type, const char* entryPoint, const std::vector<uint32_t>& binaryCode)
		{
			GLuint shaderID = glCreateShader(type);
			glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V, binaryCode.data(), (GLuint)binaryCode.size() * sizeof(uint32_t));
			glSpecializeShader(shaderID, entryPoint, 0, nullptr, nullptr);
			return shaderID;
		}

		const char* DebugName = "";
		uint32_t Program = 0;
		uint32_t RenderPipeline = 0;
	};
}