#pragma once

#include "TypeDescriptors.h"

#define GLFW_INCLUDE_NONE
#include <GL/glew.h>

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image/stb_image.h>

#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <unordered_map>

namespace HBL
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

	struct OpenGLTexture
	{
		OpenGLTexture() = default;
		OpenGLTexture(TextureDescriptor& desc) {}

		const char* DebugName = "";
		uint32_t RendererId = 0;
		glm::vec3 Dimensions = glm::vec3(0.0);
		uint32_t Format = 0;
		const char* Path = "";
	};

	struct OpenGLBuffer
	{
		OpenGLBuffer() = default;
		OpenGLBuffer(BufferDescriptor& desc)
		{
			DebugName = desc.debugName;
			ByteSize = desc.byteSize;
			Data = desc.data;

			glGenBuffers(1, &RendererId);
			glBindBuffer(GL_ARRAY_BUFFER, RendererId);
			glBufferData(GL_ARRAY_BUFFER, ByteSize, Data, GL_STATIC_DRAW);
		}

		const char* DebugName = "";
		uint32_t RendererId = 0;
		uint32_t ByteSize = 0;
		uint32_t BatchSize = 1;
		void* Data = nullptr;
	};

	struct OpenGLFramebuffer
	{
		OpenGLFramebuffer() = default;
		OpenGLFramebuffer(FramebufferDescriptor& desc) {}

		const char* DebugName = "";
		uint32_t RendererId = 0;
	};

	struct OpenGLBindGroup
	{
		OpenGLBindGroup() = default;
		OpenGLBindGroup(BindGroupDescriptor& desc) {}

		const char* DebugName = "";
	};

	struct OpenGLBindGroupLayout
	{
		OpenGLBindGroupLayout() = default;
		OpenGLBindGroupLayout(BindGroupLayoutDescriptor& desc) {}

		const char* DebugName = "";
	};

	struct OpenGLRenderPass
	{
		OpenGLRenderPass() = default;
		OpenGLRenderPass(RenderPassDescriptor& desc) {}

		const char* DebugName = "";
	};

	struct OpenGLRenderPassLayout
	{
		OpenGLRenderPassLayout() = default;
		OpenGLRenderPassLayout(RenderPassLayoutDescriptor& desc) {}

		const char* DebugName = "";
	};

	struct OpenGLMesh
	{
		OpenGLMesh() = default;
		OpenGLMesh(MeshDescriptor& desc)
		{
			DebugName = desc.debugName;
			IndexOffset = desc.indexOffset;
			IndexCount = desc.indexCount;
			VertexOffset = desc.vertexOffset;
			VertexCount = desc.vertexCount;
			IndexBuffer = desc.indexBuffer;
			VertexBuffers = desc.vertexBuffers;
		}

		const char* DebugName = "";
		uint32_t IndexOffset;
		uint32_t IndexCount;
		uint32_t VertexOffset;
		uint32_t VertexCount;
		Handle<Buffer> IndexBuffer;
		std::vector<Handle<Buffer>> VertexBuffers;
	};

	struct OpenGLMaterial
	{
		OpenGLMaterial() = default;
		OpenGLMaterial(MaterialDescriptor& desc)
		{
			DebugName = desc.debugName;
			Shader = desc.shader;
			BindGroup = desc.bindGroup;
		}

		const char* DebugName = "";
		Handle<Shader> Shader;
		Handle<BindGroup> BindGroup;
	};
}
