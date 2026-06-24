#include "OpenGLShader.h"

#include "Utilities/JobSystem.h"

namespace HBL2
{
	static void SyncVariantWithSpecializationConstant(uint32_t i, const ShaderConstant& specializationConstant, ShaderDescriptor::RenderPipeline::PackedVariant& variant)
	{
		if (i == 0) { variant.shaderConstantBool0 = (ShaderDescriptor::RenderPipeline::packed_size)specializationConstant.value.b; return; }
		if (i == 1) { variant.shaderConstantBool1 = (ShaderDescriptor::RenderPipeline::packed_size)specializationConstant.value.b; return; }
		if (i == 2) { variant.shaderConstantBool2 = (ShaderDescriptor::RenderPipeline::packed_size)specializationConstant.value.b; return; }
		if (i == 3) { variant.shaderConstantBool3 = (ShaderDescriptor::RenderPipeline::packed_size)specializationConstant.value.b; return; }
		if (i == 4) { variant.shaderConstantBool4 = (ShaderDescriptor::RenderPipeline::packed_size)specializationConstant.value.b; return; }
		if (i == 5) { variant.shaderConstantBool5 = (ShaderDescriptor::RenderPipeline::packed_size)specializationConstant.value.b; return; }
		if (i == 6) { variant.shaderConstantBool6 = (ShaderDescriptor::RenderPipeline::packed_size)specializationConstant.value.b; return; }
		if (i == 7) { variant.shaderConstantBool7 = (ShaderDescriptor::RenderPipeline::packed_size)specializationConstant.value.b; return; }
	}

	static bool GetVariantShaderConstantValueFromIndex(const ShaderDescriptor::RenderPipeline::PackedVariant& variant, uint32_t i)
	{
		if (i == 0) { return variant.shaderConstantBool0; }
		if (i == 1) { return variant.shaderConstantBool1; }
		if (i == 2) { return variant.shaderConstantBool2; }
		if (i == 3) { return variant.shaderConstantBool3; }
		if (i == 4) { return variant.shaderConstantBool4; }
		if (i == 5) { return variant.shaderConstantBool5; }
		if (i == 6) { return variant.shaderConstantBool6; }
		if (i == 7) { return variant.shaderConstantBool7; }
        
        return false;
	}

	OpenGLShader::OpenGLShader(const ShaderDescriptor&& desc)
	{
		Recompile(std::forward<const ShaderDescriptor>(desc));
	}

	void OpenGLShader::Recompile(const ShaderDescriptor&& desc)
	{
		DebugName = desc.debugName;
		m_Type = desc.type;

		// Clear m_SpecializationConstantStages.
		for (uint32_t i = 0; i < m_SpecializationConstantStages.size(); i++)
		{
			m_SpecializationConstantStages[i].Clear();
		}

		// Fill with new ones.
		for (uint32_t i = 0; i < desc.renderPipeline.specializationConstantsPerVariant.Size(); i++)
		{
			for (uint32_t j = 0; j < desc.renderPipeline.specializationConstantsPerVariant[i].Size(); j++)
			{
				auto& variant = *((ShaderDescriptor::RenderPipeline::PackedVariant*)&desc.renderPipeline.variants[i]);
				const auto& specializationConstant = desc.renderPipeline.specializationConstantsPerVariant[i][j];

				SyncVariantWithSpecializationConstant(j, specializationConstant, variant);

				m_SpecializationConstantStages[j] = specializationConstant.stage;
			}
		}

		// Copy shader code to cpu side buffers in order to be able to specialize again at runtime.
		switch (m_Type)
		{
		case ShaderType::RASTERIZATION:
			{
				m_EntryPoints[0] = desc.VS.entryPoint;
				m_EntryPoints[1] = desc.FS.entryPoint;

				m_ShaderCode[0].assign(desc.VS.code.begin(), desc.VS.code.end());
				m_ShaderCode[1].assign(desc.FS.code.begin(), desc.FS.code.end());

				// NOTE: We mark to create VAO at bind time which ensures will be called from the render thread.
				// Since VAOs are not shared between opengl contexts.
				m_NeedRenderPipelineCreation = true;

				VertexBufferBindings.clear();

				for (auto& vbb : desc.renderPipeline.vertexBufferBindings)
				{
					VertexBufferBindings.push_back(vbb);
				}
				break;
			}
		case ShaderType::COMPUTE:
			{
				m_EntryPoints[0] = desc.CS.entryPoint;
				m_ShaderCode[0].assign(desc.CS.code.begin(), desc.CS.code.end());
				break;
			}
		}

		// Create all the variants.
		for (const auto& variant : desc.renderPipeline.variants)
		{
			GetOrCreateVariant(variant, false);
		}

		if (!JobSystem::Get().IsRenderThread())
		{
			glFlush();
		}
	}

	uint64_t OpenGLShader::GetOrCreateVariant(const ShaderDescriptor::RenderPipeline::PackedVariant& variantDesc, bool flush)
	{
		for (const auto& variant : m_Variants)
		{
			if (variant.Key == variantDesc)
			{
				return variant.Key.Key();
			}
		}

		GLuint program = glCreateProgram();

		switch (m_Type)
		{
		case ShaderType::RASTERIZATION:
		{
			GLuint vs = Compile(GL_VERTEX_SHADER, "mainVS", m_ShaderCode[0], variantDesc);
			GLuint fs = Compile(GL_FRAGMENT_SHADER, "mainPS", m_ShaderCode[1], variantDesc);
			glAttachShader(program, vs);
			glAttachShader(program, fs);
			glLinkProgram(program);
			glValidateProgram(program);
			glDeleteShader(vs);
			glDeleteShader(fs);
		}
		break;
		case ShaderType::COMPUTE:
		{
			GLuint cs = Compile(GL_COMPUTE_SHADER, "mainCS", m_ShaderCode[0], variantDesc);
			glAttachShader(program, cs);
			glLinkProgram(program);
			glValidateProgram(program);
			glDeleteShader(cs);
		}
		break;
		}

		GLint linkStatus;
		glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
		if (linkStatus == GL_FALSE)
		{
			GLint logLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
			std::vector<GLchar> log(logLength);
			glGetProgramInfoLog(program, logLength, &logLength, log.data());
			HBL2_CORE_ERROR("Shader program linking failed: {}", log.data());
		}

		if (!JobSystem::Get().IsRenderThread() && flush)
		{
			glFlush();
		}

		m_Variants.push_back({ variantDesc, program });

		return variantDesc.Key();
	}

	GLuint OpenGLShader::Compile(GLuint type, const char* entryPoint, const Span<const uint32_t>& binaryCode, const ShaderDescriptor::RenderPipeline::PackedVariant& variant)
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
			BuildSpecializationInfo(ShaderStage::VERTEX, specializationData, variant);
			break;
		case GL_FRAGMENT_SHADER:
			BuildSpecializationInfo(ShaderStage::FRAGMENT, specializationData, variant);
			break;
		case GL_COMPUTE_SHADER:
			BuildSpecializationInfo(ShaderStage::COMPUTE, specializationData, variant);
			break;
		}

		if (specializationData.IsEmpty())
		{
			glSpecializeShader(shaderID, entryPoint, 0, nullptr, nullptr);
		}
		else
		{
			glSpecializeShader(shaderID, entryPoint, (GLuint)specializationData.indices.size(), specializationData.indices.data(), specializationData.values.data());
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

	void OpenGLShader::Bind(const ShaderDescriptor::RenderPipeline::PackedVariant& variantDesc)
	{
		for (const auto& variant : m_Variants)
		{
			if (variant.Key == variantDesc)
			{
				glUseProgram(variant.Program);
				return;
			}
		}
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
		for (const auto& variant : m_Variants)
		{
			glDeleteProgram(variant.Program);
		}

		m_Variants.clear();

		glDeleteVertexArrays(1, &RenderPipeline);
	}

	void OpenGLShader::BuildSpecializationInfo(ShaderStage stage, SpecializationData& specializationData, const ShaderDescriptor::RenderPipeline::PackedVariant& variant)
	{
		if (m_SpecializationConstantStages.size() == 0)
		{
			return;
		}

		GLuint constantID = 0;

		for (uint32_t i = 0; i < m_SpecializationConstantStages.size(); i++)
		{
			if (!m_SpecializationConstantStages[i].IsSet(stage))
			{
				constantID++;
				continue;
			}

			specializationData.indices.push_back(constantID++);
			GLuint value = GetVariantShaderConstantValueFromIndex(variant, i) ? 1u : 0u;
			specializationData.values.push_back(value);
		}
	}
}

