#pragma once

#include "Renderer\Enums.h"

#include <slang.h>
#include <slang-com-ptr.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace HBL2
{
    enum class ResourceType : uint8_t
    {
        UniformBuffer,   // ConstantBuffer<T>
        StorageBuffer,   // RWStructuredBuffer / StructuredBuffer
        SampledTexture,  // Texture2D / Sampler2D (combined)
        StorageTexture,  // RWTexture2D
        Sampler,         // SamplerState
        Unknown,
    };

    struct VertexAttribute
    {
        std::string  name;
        uint32_t     location;
        VertexFormat format;
        uint32_t     componentCount;
        uint32_t     sizeBytes;       // total size of the attribute in bytes
    };

    struct DescriptorBinding
    {
        std::string  name;
        uint32_t     binding;
        uint32_t     set;
        ResourceType type;
        uint32_t     count;           // array size, 1 for non-arrays
        ShaderStage  stage;           // which stage declared this binding
    };

    struct DescriptorSetLayout
    {
        uint32_t                       set;
        std::vector<DescriptorBinding> bindings;

        // Convenience: find a binding by name
        const DescriptorBinding* find(const std::string& name) const;

        // Convenience: find a binding by index
        const DescriptorBinding* findByBinding(uint32_t binding) const;
    };

    struct EntryPoint
    {
        std::string name;
        ShaderStage stage;
    };

    struct ShaderReflectionData
    {
        std::string                         sourcePath;
        std::vector<EntryPoint>             entryPoints;
        std::vector<VertexAttribute>        vertexAttributes;   // ordered by location
        std::vector<DescriptorSetLayout>    descriptorSets;     // ordered by set index

        // Returns nullptr if not found
        const EntryPoint*          findEntryPoint(ShaderStage stage) const;
        const DescriptorSetLayout* findSet(uint32_t set) const;
        const DescriptorBinding*   findBinding(const std::string& name) const;
        const VertexAttribute*     findAttribute(const std::string& name) const;
        const VertexAttribute*     findAttributeByLocation(uint32_t location) const;

        // True if any descriptor binding exists with this name
        bool hasBinding(const std::string& name) const;

        void print() const; // dumps a human-readable summary to stdout
    };

    class ShaderReflector
    {
    public:
        // Primary entry point: call after linking your Slang program.
        // linkedProgram must be the result of IComponentType::link().
        static ShaderReflectionData Reflect(slang::IComponentType* linkedProgram, const std::string& sourcePath = "");

    private:
        static void ReflectEntryPoints(slang::ProgramLayout* layout, ShaderReflectionData&  out);
        static void ReflectVertexAttributes(slang::ProgramLayout* layout, ShaderReflectionData&  out);
        static void ReflectDescriptorSets(slang::ProgramLayout* layout, ShaderReflectionData&  out);

        // --- Type helpers ---
        friend class ShaderReflectionData;

        static ShaderStage   ToShaderStage(SlangStage stage);
        static ResourceType  ToResourceType(slang::TypeReflection* type);
        static VertexFormat  ToVertexFormat(slang::TypeReflection* type);
        static uint32_t      ComponentCount(VertexFormat format);
        static uint32_t      FormatSizeBytes(VertexFormat format);
        static const char*   VertexFormatToString(VertexFormat format);
        static const char*   ResourceTypeToString(ResourceType type);
        static const char*   ShaderStageToString(ShaderStage stage);
    };

} // namespace HBL2
