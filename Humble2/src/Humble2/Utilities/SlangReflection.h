#pragma once

#include "ShaderReflectionTypes.h"

#include <slang.h>
#include <slang-com-ptr.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace HBL2
{
    class ShaderReflector
    {
    public:
        // Primary entry point: call after linking your Slang program.
        // linkedProgram must be the result of IComponentType::link().
        static ShaderReflectionData Reflect(slang::IComponentType* linkedProgram, const std::string& sourcePath = "");
        static ComputeShaderReflectionData ReflectCompute(slang::IComponentType* linkedProgram, const std::string& sourcePath = "");

    private:
        static void ReflectEntryPoints(slang::ProgramLayout* layout, ShaderReflectionData&  out);
        static void ReflectVertexAttributes(slang::ProgramLayout* layout, ShaderReflectionData&  out);
        static void ReflectDescriptorSets(slang::IComponentType* linkedProgram, slang::ProgramLayout* layout, ShaderReflectionData&  out);
        static void ReflectSpecializationConstants(slang::IComponentType* linkedProgram, slang::ProgramLayout* layout, ShaderReflectionData& out);

        // Type helpers
        friend class ShaderReflectionData;

        static ShaderStage ToShaderStage(SlangStage stage);
        static ShaderResourceType ToResourceType(slang::TypeReflection* type);
        static VertexFormat ToVertexFormat(slang::TypeReflection* type);
        static uint32_t ComponentCount(VertexFormat format);
        static uint32_t FormatSizeBytes(VertexFormat format);
        static const char* VertexFormatToString(VertexFormat format);
        static const char* ResourceTypeToString(ShaderResourceType type);
        static const char* ShaderStageToString(BitFlags<ShaderStage> stageMask);
        static ShaderConstantType ToConstantType(slang::TypeReflection* type);
        static const char* ConstantTypeToString(ShaderConstantType type);
    };

} // namespace HBL2
