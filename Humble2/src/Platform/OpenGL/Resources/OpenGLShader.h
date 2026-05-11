#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Platform\OpenGL\OpenGLCommon.h"

#include "Utilities\Collections\Span.h"
#include "Utilities\Collections\StaticDArray.h"

#include <string>
#include <fstream>
#include <sstream>
#include <stdint.h>

namespace HBL2
{
	struct OpenGLShader
	{
		OpenGLShader() = default;
		OpenGLShader(const ShaderDescriptor&& desc);

		void Recompile(const ShaderDescriptor&& desc);
		GLuint Compile(GLuint type, const char* entryPoint, const Span<const uint32_t>& binaryCode, const Span<const ShaderConstant>& constants);
		void SetVariantProperties(const ShaderDescriptor::RenderPipeline::PackedVariant& variantDesc);
		void Bind();
		void BindPipeline();
		void Destroy();

		const char* DebugName = "";
		GLuint Program = 0;
		GLuint RenderPipeline = 0;
		std::vector<ShaderDescriptor::RenderPipeline::VertexBufferBinding> VertexBufferBindings;

	private:
		struct SpecializationData
		{
			StaticDArray<GLuint, 8> indices;
			StaticDArray<GLuint, 8> values;
		};

		void BuildSpecializationInfo(ShaderStage stage, SpecializationData& specializationData, Span<const ShaderConstant> constants);

		bool m_NeedRenderPipelineCreation = false;
	};
}