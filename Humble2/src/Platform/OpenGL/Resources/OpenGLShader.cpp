#include "OpenGLShader.h"

#include "Utilities\JobSystem.h"

namespace HBL2
{
	OpenGLShader::OpenGLShader(const ShaderDescriptor&& desc)
	{
		Recompile(std::forward<const ShaderDescriptor>(desc));
	}

	void OpenGLShader::Recompile(const ShaderDescriptor&& desc)
	{
		DebugName = desc.debugName;

		Program = glCreateProgram();

		switch (desc.type)
		{
		case ShaderType::RASTERIZATION:
			{
				GLuint vs = Compile(GL_VERTEX_SHADER, desc.VS.entryPoint, desc.VS.code, desc.renderPipeline.specializationConstants);
				GLuint fs = Compile(GL_FRAGMENT_SHADER, desc.FS.entryPoint, desc.FS.code, desc.renderPipeline.specializationConstants);
				glAttachShader(Program, vs);
				glAttachShader(Program, fs);
				glLinkProgram(Program);
				glValidateProgram(Program);
				glDeleteShader(vs);
				glDeleteShader(fs);

				// NOTE: We mark to create VAO at bind time which ensures will be called from the render thread.
				// Since VAOs are not shared between opengl contexts.
				m_NeedRenderPipelineCreation = true;

				for (auto& vbb : desc.renderPipeline.vertexBufferBindings)
				{
					VertexBufferBindings.push_back(vbb);
				}
			}
			break;
		case ShaderType::COMPUTE:
			{
				GLuint cs = Compile(GL_COMPUTE_SHADER, desc.CS.entryPoint, desc.CS.code, desc.renderPipeline.specializationConstants);
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
			HBL2_CORE_ERROR("Shader Program linking failed: {}", log.data());
		}

		if (!JobSystem::Get().IsRenderThread())
		{
			glFlush();
		}
	}

	GLuint OpenGLShader::Compile(GLuint type, const char* entryPoint, const Span<const uint32_t>& binaryCode, const Span<const ShaderConstant>& constants)
	{
		if (binaryCode.Size() == 0)
		{
			return 0;
		}

		GLuint shaderID = glCreateShader(type);
		glShaderBinary(1, &shaderID, GL_SHADER_BINARY_FORMAT_SPIR_V, binaryCode.Data(), (GLuint)binaryCode.Size() * sizeof(uint32_t));

		SpecializationData specializationData;

		switch (type)
		{
		case GL_VERTEX_SHADER:
			BuildSpecializationInfo(ShaderStage::VERTEX, specializationData, constants);
			glSpecializeShader(shaderID, entryPoint, (GLuint)specializationData.indices.size(), specializationData.indices.data(), specializationData.values.data());
			break;
		case GL_FRAGMENT_SHADER:
			BuildSpecializationInfo(ShaderStage::FRAGMENT, specializationData, constants);
			glSpecializeShader(shaderID, entryPoint, (GLuint)specializationData.indices.size(), specializationData.indices.data(), specializationData.values.data());
			break;
		case GL_COMPUTE_SHADER:
			BuildSpecializationInfo(ShaderStage::COMPUTE, specializationData, constants);
			glSpecializeShader(shaderID, entryPoint, (GLuint)specializationData.indices.size(), specializationData.indices.data(), specializationData.values.data());
			break;
		}

		GLint compileStatus;
		glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);
		if (compileStatus == GL_FALSE)
		{
			GLint logLength = 0;
			glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);
			std::vector<GLchar> log(logLength);
			glGetShaderInfoLog(shaderID, logLength, &logLength, log.data());
			HBL2_CORE_ERROR("Shader compilation failed: {}", log.data());
		}

		return shaderID;
	}

	void OpenGLShader::SetVariantProperties(const ShaderDescriptor::RenderPipeline::PackedVariant& variantDesc)
	{
		// Blend state.
		if (variantDesc.blendEnabled)
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
		if (variantDesc.depthEnabled)
		{
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(OpenGLUtils::CompareToGLenum((Compare)variantDesc.depthCompare));
		}
		else
		{
			glDisable(GL_DEPTH_TEST);
		}

		if (variantDesc.depthWrite)
		{
			glDepthMask(GL_TRUE);
		}
		else
		{
			glDepthMask(GL_FALSE);
		}

		// Cull mode.
		switch ((CullMode)variantDesc.cullMode)
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

		if ((CullMode)variantDesc.cullMode != CullMode::NONE)
		{
			switch ((FrontFace)variantDesc.frontFace)
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
		switch ((PolygonMode)variantDesc.polygonMode)
		{
		case PolygonMode::FILL:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			break;
		case PolygonMode::LINE:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			break;
		case PolygonMode::POINT:
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			break;
		}
	}

	void OpenGLShader::Bind()
	{
		glUseProgram(Program);
	}

	void OpenGLShader::BindPipeline()
	{
		if (m_NeedRenderPipelineCreation)
		{
			glGenVertexArrays(1, &RenderPipeline);
			m_NeedRenderPipelineCreation = false;
		}

		glBindVertexArray(RenderPipeline);
	}

	void OpenGLShader::Destroy()
	{
		glDeleteProgram(Program);
		glDeleteVertexArrays(1, &RenderPipeline);
	}

	void OpenGLShader::BuildSpecializationInfo(ShaderStage stage, SpecializationData& specializationData, Span<const ShaderConstant> constants)
	{
		specializationData.indices.clear();
		specializationData.values.clear();

		if (constants.Size() == 0)
		{
			return;
		}

		GLuint constantID = 0;

		for (const auto& constant : constants)
		{
			if (constant.stage != stage)
			{
				continue;
			}

			specializationData.indices.push_back(constantID++);

			GLuint value = 0;

			switch (constant.type)
			{
			case ShaderConstantType::Bool:
				value = constant.value.b ? 1u : 0u;
				break;

			case ShaderConstantType::Int:
				value = static_cast<GLuint>(constant.value.i);
				break;

			case ShaderConstantType::UInt:
				value = constant.value.u;
				break;

			case ShaderConstantType::Float:
				static_assert(sizeof(float) == sizeof(uint32_t));
				std::memcpy(&value, &constant.value.f, sizeof(float));
				break;
			}

			specializationData.values.push_back(value);
		}
	}
}

