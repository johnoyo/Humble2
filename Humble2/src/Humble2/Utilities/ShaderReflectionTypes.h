#pragma once

#include "Renderer/Enums.h"
#include "Resources/TypeDescriptors.h"
#include "Utilities/Collections/BitFlags.h"

#include <string>
#include <vector>

namespace HBL2
{
    enum class ShaderResourceType : uint8_t
    {
        UniformBuffer,   // ConstantBuffer<T>
        StorageBuffer,   // RWStructuredBuffer / StructuredBuffer
        StorageBufferReadOnly,   // RWStructuredBuffer / StructuredBuffer
        SampledTexture,  // Texture2D
        CombinedTextureSampler,  // Sampler2D (combined)
        StorageTexture,  // RWTexture2D
        Sampler,         // SamplerState
        Unknown,
    };

    struct VertexAttribute
    {
        std::string name;
        uint32_t location;
        VertexFormat format;
        uint32_t componentCount;
        uint32_t sizeBytes;       // total size of the attribute in bytes
    };

    enum class MemberBaseType { Unknown, Bool, Int, UInt, Float, Double };

    struct MemberTypeInfo
    {
        MemberBaseType base = MemberBaseType::Unknown;
        uint32_t       rows = 1;   // 1 for scalars/vectors
        uint32_t       cols = 1;   // >1 for vectors/matrices
        bool           isArray = false;
        uint32_t       arrayCount = 0;
    };

    struct UniformMember
    {
        std::string    name;
        uint32_t       offset;     // byte offset within the struct
        uint32_t       size;       // byte size
        MemberTypeInfo typeInfo;
    };

    struct DescriptorBinding
    {
        std::string name;
        uint32_t binding;
        uint32_t set;
        ShaderResourceType type;
        uint32_t count;           // array size, 1 for non-arrays
        uint64_t size;       // byte size of the underlying type; 0 for opaque resources
        std::vector<UniformMember> members;
        BitFlags<ShaderStage> stageMask; // which stages declared this binding
    };

    struct DescriptorSetLayout
    {
        uint32_t set;
        std::vector<DescriptorBinding> bindings;

        // Find a binding by name
        const DescriptorBinding* find(const std::string& name) const;

        // Find a binding by index
        const DescriptorBinding* findByBinding(uint32_t binding) const;
    };

    struct EntryPoint
    {
        std::string name;
        ShaderStage stage;
    };

    struct ReflectedSpecializationConstant
    {
        std::string name;
        uint32_t constantId;   // Vulkan constant_id / SPIR-V specialization index
        ShaderConstantType type;
        BitFlags<ShaderStage> stageMask;
    };

    struct HBL2_API ShaderReflectionData
    {
        std::string sourcePath;
        std::vector<EntryPoint> entryPoints;
        std::vector<VertexAttribute> vertexAttributes;   // ordered by location
        std::vector<ShaderDescriptor::RenderPipeline::VertexBufferBinding> vertexBufferBindings; // one per VSInput struct parameter
        std::vector<DescriptorSetLayout> descriptorSets;     // ordered by set index
        std::vector<ReflectedSpecializationConstant> specializationConstants; // ordered by constantId

        Span<const Span<const ShaderConstant>> GetSpecializationConstantsPerVariant(Span<const ShaderDescriptor::RenderPipeline::PackedVariant> variants);

        // Returns nullptr if not found
        const EntryPoint* findEntryPoint(ShaderStage stage) const;
        const DescriptorSetLayout* findSet(uint32_t set) const;
        const DescriptorBinding* findBinding(const std::string& name) const;
        const VertexAttribute* findAttribute(const std::string& name) const;
        const VertexAttribute* findAttributeByLocation(uint32_t location) const;
        const ShaderDescriptor::RenderPipeline::VertexBufferBinding* findVertexBufferBinding(uint32_t bufferBinding) const;
        const ReflectedSpecializationConstant* findSpecializationConstant(const std::string& name) const;
        const ReflectedSpecializationConstant* findSpecializationConstantById(uint32_t constantId) const;

        Handle<BindGroupLayout> GetBindGroupLayout(uint32_t set);

        // True if any descriptor binding exists with this name
        bool hasBinding(const std::string& name) const;

        void print() const; // dumps a human-readable summary to stdout

        void Clear();

    private:
        // Backing storage for GetSpecializationConstantsPerVariant
        std::vector<ShaderConstant> m_SpecConstantStorage;
        std::vector<Span<const ShaderConstant>> m_SpecConstantsPerVariant;
    };

    struct ComputeShaderReflectionData
    {
        glm::u16vec3 threadsPerThreadGroup = {};
    };
}