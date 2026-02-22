#pragma once

#include "Base.h"
#include "Resources\TypeDescriptors.h"

#include "Utilities\Collections\Span.h"
#include "Platform\OpenGL\OpenGLCommon.h"

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
		GLuint Compile(GLuint type, const char* entryPoint, const Span<const uint32_t>& binaryCode);
		void SetVariantProperties(const ShaderDescriptor::RenderPipeline::PackedVariant& variantDesc);
		void Bind();
		void BindPipeline();
		void Destroy();

		const char* DebugName = "";
		GLuint Program = 0;
		GLuint RenderPipeline = 0;
		std::vector<ShaderDescriptor::RenderPipeline::VertexBufferBinding> VertexBufferBindings;
	};
}