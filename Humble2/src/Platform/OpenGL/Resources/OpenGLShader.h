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

		uint64_t GetOrCreateVariant(const ShaderDescriptor::RenderPipeline::PackedVariant& variantDesc, bool flush = true);

		GLuint Compile(GLuint type, const char* entryPoint, const Span<const uint32_t>& binaryCode, const ShaderDescriptor::RenderPipeline::PackedVariant& variant);
		void SetVariantProperties(const ShaderDescriptor::RenderPipeline::PackedVariant& variantDesc);
		void Bind(const ShaderDescriptor::RenderPipeline::PackedVariant& variantDesc);
		void BindPipeline();
		void Destroy();

		const char* DebugName = "";
		GLuint RenderPipeline = 0;
		std::vector<ShaderDescriptor::RenderPipeline::VertexBufferBinding> VertexBufferBindings;

	private:
		static constexpr uint32_t MaxVariants = 8;
		static constexpr uint32_t MaxSpecializationConstants = 8;
		
		struct SpecializationData
		{
			StaticDArray<GLuint, MaxSpecializationConstants> indices;
			StaticDArray<GLuint, MaxSpecializationConstants> values;

			bool IsEmpty()
			{
				return indices.empty() && values.empty();
			}
		};

		struct Variant
		{
			ShaderDescriptor::RenderPipeline::PackedVariant Key;
			GLuint Program;
		};

		void BuildSpecializationInfo(ShaderStage stage, SpecializationData& specializationData, const ShaderDescriptor::RenderPipeline::PackedVariant& variant);

		StaticDArray<Variant, MaxVariants> m_Variants;
		std::array<BitFlags<ShaderStage>, MaxSpecializationConstants> m_SpecializationConstantStages;

		std::array<const char*, 2> m_EntryPoints;
		std::array<std::vector<uint32_t>, 2> m_ShaderCode;

		ShaderType m_Type = ShaderType::RASTERIZATION;
		bool m_NeedRenderPipelineCreation = false;
	};
}