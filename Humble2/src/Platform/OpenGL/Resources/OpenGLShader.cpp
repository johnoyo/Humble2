#include "OpenGLShader.h"

namespace HBL2
{
	OpenGLShader::OpenGLShader(const ShaderDescriptor&& desc)
	{
		DebugName = desc.debugName;

		Program = glCreateProgram();

		switch (desc.type)
		{
		case ShaderType::RASTERIZATION:
			{
				GLuint vs = Compile(GL_VERTEX_SHADER, desc.VS.entryPoint, desc.VS.code);
				GLuint fs = Compile(GL_FRAGMENT_SHADER, desc.FS.entryPoint, desc.FS.code);
				glAttachShader(Program, vs);
				glAttachShader(Program, fs);
				glLinkProgram(Program);
				glValidateProgram(Program);
				glDeleteShader(vs);
				glDeleteShader(fs);

				glGenVertexArrays(1, &RenderPipeline);
				glBindVertexArray(RenderPipeline);
				VertexBufferBindings = desc.renderPipeline.vertexBufferBindings;
			}
			break;
		case ShaderType::COMPUTE:
			{
				GLuint cs = Compile(GL_COMPUTE_SHADER, desc.CS.entryPoint, desc.CS.code);
				glAttachShader(Program, cs);
				glLinkProgram(Program);
				glValidateProgram(Program);
				glDeleteShader(cs);
			}
			break;
		}

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
	}

	GLuint OpenGLShader::Compile(GLuint type, const char* entryPoint, const Span<const uint32_t>& binaryCode)
	{
		if (binaryCode.Size() == 0)
		{
			return 0;
		}

		GLuint shaderID = glCreateShader(type);
		glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V, binaryCode.Data(), (GLuint)binaryCode.Size() * sizeof(uint32_t));
		glSpecializeShader(shaderID, entryPoint, 0, nullptr, nullptr);

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

	void OpenGLShader::SetVariantProperties(const ShaderDescriptor::RenderPipeline::Variant& variantDesc)
	{
		// Blend state.
		if (variantDesc.blend.enabled)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBlendEquation(GL_FUNC_ADD);
		}
		else
		{
			glDisable(GL_BLEND);
		}

		// Depth test state.
		if (variantDesc.depthTest.enabled)
		{
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(OpenGLUtils::CompareToGLenum(variantDesc.depthTest.depthTest));
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
		}

		if (variantDesc.depthTest.writeEnabled)
		{
			glDepthMask(GL_TRUE);
		}
		else
		{
			glDepthMask(GL_FALSE);
		}

		// Cull mode.
		switch (variantDesc.cullMode)
		{
		case CullMode::NONE:
			break;
		case CullMode::BACK:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			break;
		case CullMode::FRONT:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			break;
		case CullMode::FRONT_AND_BACK:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT_AND_BACK);
			break;
		}

		if (variantDesc.cullMode != CullMode::NONE)
		{
			switch (variantDesc.frontFace)
			{
			case FrontFace::COUNTER_CLOCKWISE:
				glFrontFace(GL_CW); // NOTE: Flipped to match vulkan set up!
				break;
			case FrontFace::CLOCKWISE:
				glFrontFace(GL_CCW);
				break;
			}
		}

		// Polygon mode.
		switch (variantDesc.polygonMode)
		{
		case PolygonMode::FILL:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		case PolygonMode::LINE:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		}
	}

	void OpenGLShader::Bind()
	{
		glUseProgram(Program);
	}

	void OpenGLShader::BindPipeline()
	{
		glBindVertexArray(RenderPipeline);
	}

	void OpenGLShader::Destroy()
	{
		glDeleteProgram(Program);
		glDeleteVertexArrays(1, &RenderPipeline);
	}
}

