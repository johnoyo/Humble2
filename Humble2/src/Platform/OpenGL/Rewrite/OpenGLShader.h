#pragma once

#include "Base.h"
#include "Renderer\Rewrite\TypeDescriptors.h"

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
		OpenGLShader(ShaderDescriptor& desc)
		{
			std::fstream stream;

			std::string line;
			std::stringstream ssV;

			stream.open(desc.VS.code, std::ios::in);

			if (stream.is_open())
			{
				while (getline(stream, line))
				{
					ssV << line << '\n';
				}

				stream.close();
			}
			else
			{
				HBL2_CORE_ERROR("Could not open file: {0}.", desc.VS.code);
			}

			line.clear();
			std::stringstream ssF;

			stream.open(desc.FS.code, std::ios::in);

			if (stream.is_open())
			{
				while (getline(stream, line))
				{
					ssF << line << '\n';
				}

				stream.close();
			}
			else
			{
				HBL2_CORE_ERROR("Could not open file: {0}.", desc.FS.code);
			}

			Program = glCreateProgram();
			uint32_t vs = Compile(GL_VERTEX_SHADER, ssV.str());
			uint32_t fs = Compile(GL_FRAGMENT_SHADER, ssF.str());

			glAttachShader(Program, vs);
			glAttachShader(Program, fs);
			glLinkProgram(Program);
			glValidateProgram(Program);

			glDeleteShader(vs);
			glDeleteShader(fs);

			glGenVertexArrays(1, &RenderPipeline);
			glBindVertexArray(RenderPipeline);
		}

		uint32_t Compile(uint32_t type, const std::string& source)
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
				HBL2_CORE_ERROR("Failed to compile {0} shader.", (type == GL_VERTEX_SHADER ? "vertex" : "fragment"));
				HBL2_CORE_ERROR(message);
				glDeleteShader(id);

				return 0;
			}

			return id;
		}

		const char* DebugName = "";
		uint32_t Program = 0;
		uint32_t RenderPipeline = 0;
	};
}