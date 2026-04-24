#include "SlangReflection.h"

#include <algorithm>
#include <cassert>
#include <cstdio>

namespace HBL2
{
    const DescriptorBinding* DescriptorSetLayout::find(const std::string& name) const
    {
        for (const auto& b : bindings)
        {
            if (b.name == name)
            {
                return &b;
            }
        }
        return nullptr;
    }

    const DescriptorBinding* DescriptorSetLayout::findByBinding(uint32_t binding) const
    {
        for (const auto& b : bindings)
        {
            if (b.binding == binding)
            {
                return &b;
            }
        }
        return nullptr;
    }

    const EntryPoint* ShaderReflectionData::findEntryPoint(ShaderStage stage) const
    {
        for (const auto& ep : entryPoints)
        {
            if (ep.stage == stage)
            {
                return &ep;
            }
        }
        return nullptr;
    }

    const DescriptorSetLayout* ShaderReflectionData::findSet(uint32_t set) const
    {
        for (const auto& s : descriptorSets)
        {
            if (s.set == set)
            {
                return &s;
            }
        }
        return nullptr;
    }

    const DescriptorBinding* ShaderReflectionData::findBinding(const std::string& name) const
    {
        for (const auto& set : descriptorSets)
        {
            if (auto* b = set.find(name))
            {
                return b;
            }
        }
        return nullptr;
    }

    const VertexAttribute* ShaderReflectionData::findAttribute(const std::string& name) const
    {
        for (const auto& a : vertexAttributes)
        {
            if (a.name == name)
            {
                return &a;
            }
        }
        return nullptr;
    }

    const VertexAttribute* ShaderReflectionData::findAttributeByLocation(uint32_t location) const
    {
        for (const auto& a : vertexAttributes)
        {
            if (a.location == location)
            {
                return &a;
            }
        }
        return nullptr;
    }

    bool ShaderReflectionData::hasBinding(const std::string& name) const
    {
        return findBinding(name) != nullptr;
    }

    void ShaderReflectionData::print() const
    {
        printf("=== Shader Reflection: %s ===\n\n", sourcePath.c_str());

        // --- Entry points ---
        printf("Entry Points (%zu):\n", entryPoints.size());
        for (const auto& ep : entryPoints)
            printf("  [%s]  %s\n", ShaderReflector::ShaderStageToString(ep.stage), ep.name.c_str());
        printf("\n");

        // --- Vertex attributes ---
        printf("Vertex Attributes (%zu):\n", vertexAttributes.size());
        for (const auto& a : vertexAttributes)
        {
            printf("  location=%-2u  %-12s  %-8s  components=%-2u  size=%u bytes\n",
                a.location,
                a.name.c_str(),
                ShaderReflector::VertexFormatToString(a.format),
                a.componentCount,
                a.sizeBytes);
        }
        printf("\n");

        // --- Descriptor sets ---
        printf("Descriptor Sets (%zu):\n", descriptorSets.size());
        for (const auto& set : descriptorSets)
        {
            printf("  Set %u:\n", set.set);
            for (const auto& b : set.bindings)
            {
                printf("    binding=%-2u  %-24s  %-16s  stage=%-8s  count=%u\n",
                    b.binding,
                    b.name.c_str(),
                    ShaderReflector::ResourceTypeToString(b.type),
                    ShaderReflector::ShaderStageToString(b.stage),
                    b.count);
            }
        }
        printf("\n");
    }

    ShaderReflectionData ShaderReflector::Reflect(slang::IComponentType* linkedProgram, const std::string& sourcePath)
    {
        assert(linkedProgram && "linkedProgram must not be null");

        slang::ProgramLayout* layout = linkedProgram->getLayout();
        assert(layout && "Failed to get program layout from Slang");

        ShaderReflectionData data;
        data.sourcePath = sourcePath;

        ReflectEntryPoints(layout, data);
        ReflectVertexAttributes(layout, data);
        ReflectDescriptorSets(layout, data);

        return data;
    }

    void ShaderReflector::ReflectEntryPoints(slang::ProgramLayout* layout, ShaderReflectionData& out)
    {
        const uint32_t count = static_cast<uint32_t>(layout->getEntryPointCount());

        out.entryPoints.reserve(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            slang::EntryPointReflection* ep = layout->getEntryPointByIndex(i);

            EntryPoint entry;
            entry.name = ep->getName();
            entry.stage = ToShaderStage(ep->getStage());

            out.entryPoints.push_back(std::move(entry));
        }
    }

    void ShaderReflector::ReflectVertexAttributes(slang::ProgramLayout* layout, ShaderReflectionData& out)
    {
        // Vertex attributes live on the vertex entry point's varying parameters
        slang::EntryPointReflection* vertexEP = nullptr;

        for (uint32_t i = 0; i < layout->getEntryPointCount(); ++i)
        {
            slang::EntryPointReflection* ep = layout->getEntryPointByIndex(i);
            if (ep->getStage() == SLANG_STAGE_VERTEX)
            {
                vertexEP = ep;
                break;
            }
        }

        if (!vertexEP)
        {
            return;
        }

        // Each parameter of the vertex entry point may be a scalar or a struct.
        // We flatten structs so every leaf varying input becomes one VertexAttribute.
        const uint32_t paramCount = static_cast<uint32_t>(vertexEP->getParameterCount());

        for (uint32_t i = 0; i < paramCount; ++i)
        {
            slang::VariableLayoutReflection* param = vertexEP->getParameterByIndex(i);
            slang::TypeLayoutReflection* typeLayout = param->getTypeLayout();
            slang::TypeReflection* type = typeLayout->getType();

            if (type->getKind() == slang::TypeReflection::Kind::Struct)
            {
                // Flatten struct fields into individual attributes
                const uint32_t fieldCount = static_cast<uint32_t>(type->getFieldCount());

                for (uint32_t f = 0; f < fieldCount; ++f)
                {
                    slang::VariableReflection* field = type->getFieldByIndex(f);
                    slang::VariableLayoutReflection* fieldLayout = typeLayout->getFieldByIndex(f);

                    // Only include varyings that have an explicit location
                    int32_t location = static_cast<int32_t>(fieldLayout->getOffset(SLANG_PARAMETER_CATEGORY_VARYING_INPUT));

                    // Slang returns ~0u if no varying semantic is bound
                    if (location == ~0u || location < 0)
                    {
                        continue;
                    }

                    VertexFormat fmt = ToVertexFormat(field->getType());

                    VertexAttribute attr;
                    attr.name = field->getName();
                    attr.location = static_cast<uint32_t>(location);
                    attr.format = fmt;
                    attr.componentCount = ComponentCount(fmt);
                    attr.sizeBytes = FormatSizeBytes(fmt);

                    out.vertexAttributes.push_back(std::move(attr));
                }
            }
            else
            {
                // Direct scalar/vector input (uncommon but valid)
                int32_t location = static_cast<int32_t>(param->getOffset(SLANG_PARAMETER_CATEGORY_VARYING_INPUT));

                if (location == ~0u || location < 0)
                {
                    continue;
                }

                VertexFormat fmt = ToVertexFormat(type);

                VertexAttribute attr;
                attr.name = param->getName();
                attr.location = static_cast<uint32_t>(location);
                attr.format = fmt;
                attr.componentCount = ComponentCount(fmt);
                attr.sizeBytes = FormatSizeBytes(fmt);

                out.vertexAttributes.push_back(std::move(attr));
            }
        }

        // Sort attributes by location for deterministic ordering
        std::sort(out.vertexAttributes.begin(), out.vertexAttributes.end(),
            [](const VertexAttribute& a, const VertexAttribute& b)
            {
                return a.location < b.location;
            });
    }

    void ShaderReflector::ReflectDescriptorSets(slang::ProgramLayout* layout, ShaderReflectionData& out)
    {
        // Global parameters hold all descriptor resources (UBOs, textures, etc.)
        const uint32_t paramCount = static_cast<uint32_t>(layout->getParameterCount());

        // Accumulate into a map keyed by set index, then flatten at the end
        std::unordered_map<uint32_t, DescriptorSetLayout> setMap;

        for (uint32_t i = 0; i < paramCount; ++i)
        {
            slang::VariableLayoutReflection* param = layout->getParameterByIndex(i);

            const uint32_t binding = static_cast<uint32_t>(param->getBindingIndex());
            const uint32_t set = static_cast<uint32_t>(param->getBindingSpace());

            slang::TypeReflection* type = param->getType();

            // Array resources: unwrap to get the element type and count
            uint32_t arrayCount = 1;
            if (type->getKind() == slang::TypeReflection::Kind::Array)
            {
                arrayCount = static_cast<uint32_t>(type->getElementCount());
                type = type->getElementType();
            }

            ResourceType resourceType = ToResourceType(type);
            if (resourceType == ResourceType::Unknown)
            {
                continue; // skip push constants, plain structs, etc.
            }

            // Determine which stage this binding is used in by checking
            // if it appears in any entry point's parameter list
            ShaderStage usedInStage = ShaderStage::NONE;
            for (uint32_t ep = 0; ep < layout->getEntryPointCount(); ++ep)
            {
                slang::EntryPointReflection* entry = layout->getEntryPointByIndex(ep);
                const uint32_t epParamCount = static_cast<uint32_t>(entry->getParameterCount());

                for (uint32_t p = 0; p < epParamCount; ++p)
                {
                    slang::VariableLayoutReflection* epParam = entry->getParameterByIndex(p);
                    if (std::string(epParam->getName()) == param->getName())
                    {
                        usedInStage = ToShaderStage(entry->getStage());
                        break;
                    }
                }

                if (usedInStage != ShaderStage::NONE)
                {
                    break;
                }
            }

            DescriptorBinding descBinding;
            descBinding.name = param->getName();
            descBinding.binding = binding;
            descBinding.set = set;
            descBinding.type = resourceType;
            descBinding.count = arrayCount;
            descBinding.stage = usedInStage;

            setMap[set].set = set;
            setMap[set].bindings.push_back(std::move(descBinding));
        }

        // Flatten map → vector, sorted by set index
        out.descriptorSets.reserve(setMap.size());
        for (auto& [setIndex, setLayout] : setMap)
        {
            // Sort bindings within each set by binding index
            std::sort(setLayout.bindings.begin(), setLayout.bindings.end(),
                [](const DescriptorBinding& a, const DescriptorBinding& b)
                {
                    return a.binding < b.binding;
                });

            out.descriptorSets.push_back(std::move(setLayout));
        }

        std::sort(out.descriptorSets.begin(), out.descriptorSets.end(),
            [](const DescriptorSetLayout& a, const DescriptorSetLayout& b)
            {
                return a.set < b.set;
            });
    }

    ShaderStage ShaderReflector::ToShaderStage(SlangStage stage)
    {
        switch (stage)
        {
        case SLANG_STAGE_VERTEX:   return ShaderStage::VERTEX;
        case SLANG_STAGE_FRAGMENT: return ShaderStage::FRAGMENT;
        case SLANG_STAGE_COMPUTE:  return ShaderStage::COMPUTE;
        default:                   return ShaderStage::NONE;
        }
    }

    ResourceType ShaderReflector::ToResourceType(slang::TypeReflection* type)
    {
        using Kind = slang::TypeReflection::Kind;

        switch (type->getKind())
        {
        case Kind::ConstantBuffer:
            return ResourceType::UniformBuffer;

        case Kind::Resource:
        {
            SlangResourceAccess access = type->getResourceAccess();
            SlangResourceShape  shape  = (SlangResourceShape)(type->getResourceShape() & SLANG_RESOURCE_BASE_SHAPE_MASK);

            const bool isSampler = (type->getResourceShape() & SLANG_TEXTURE_COMBINED_FLAG) != 0 || shape == SLANG_TEXTURE_2D;

            if (access == SLANG_RESOURCE_ACCESS_READ_WRITE)
            {
                if (shape == SLANG_TEXTURE_2D || shape == SLANG_TEXTURE_3D || shape == SLANG_TEXTURE_CUBE || shape == SLANG_TEXTURE_2D_ARRAY)
                {
                    return ResourceType::StorageTexture;
                }

                return ResourceType::StorageBuffer;
            }

            if (shape == SLANG_TEXTURE_2D || shape == SLANG_TEXTURE_3D || shape == SLANG_TEXTURE_CUBE || shape == SLANG_TEXTURE_2D_ARRAY)
            {
                return ResourceType::SampledTexture;
            }

            if (shape == SLANG_STRUCTURED_BUFFER || shape == SLANG_BYTE_ADDRESS_BUFFER)
            {
                return ResourceType::StorageBuffer;
            }

            return ResourceType::Unknown;
        }

        case Kind::SamplerState:
            return ResourceType::Sampler;

        default:
            return ResourceType::Unknown;
        }
    }

    VertexFormat ShaderReflector::ToVertexFormat(slang::TypeReflection* type)
    {
        using Kind = slang::TypeReflection::Kind;

        if (type->getKind() == Kind::Scalar)
        {
            switch (type->getScalarType())
            {
            case SLANG_SCALAR_TYPE_FLOAT32: return VertexFormat::FLOAT32;
            case SLANG_SCALAR_TYPE_INT32:   return VertexFormat::INT32;
            case SLANG_SCALAR_TYPE_UINT32:  return VertexFormat::UINT32;
            default:                        return VertexFormat::NONE;
            }
        }

        if (type->getKind() == Kind::Vector)
        {
            slang::TypeReflection* elem  = type->getElementType();
            const uint32_t         count = static_cast<uint32_t>(type->getElementCount());

            switch (elem->getScalarType())
            {
            case SLANG_SCALAR_TYPE_FLOAT32:
                if (count == 2) return VertexFormat::FLOAT32x2;
                if (count == 3) return VertexFormat::FLOAT32x3;
                if (count == 4) return VertexFormat::FLOAT32x4;
                break;

            case SLANG_SCALAR_TYPE_INT32:
                if (count == 2) return VertexFormat::INT32x2;
                if (count == 3) return VertexFormat::INT32x3;
                if (count == 4) return VertexFormat::INT32x4;
                break;

            case SLANG_SCALAR_TYPE_UINT32:
                if (count == 2) return VertexFormat::UINT32x2;
                if (count == 3) return VertexFormat::UINT32x3;
                if (count == 4) return VertexFormat::UINT32x4;
                break;

            default: break;
            }
        }

        return VertexFormat::NONE;
    }

    uint32_t ShaderReflector::ComponentCount(VertexFormat format)
    {
        switch (format)
        {
        case VertexFormat::FLOAT32:
        case VertexFormat::INT32:
        case VertexFormat::UINT32:
            return 1;
        case VertexFormat::FLOAT32x2:
        case VertexFormat::INT32x2:
        case VertexFormat::UINT32x2:
            return 2;
        case VertexFormat::FLOAT32x3:
        case VertexFormat::INT32x3:
        case VertexFormat::UINT32x3:
            return 3;
        case VertexFormat::FLOAT32x4:
        case VertexFormat::INT32x4:
        case VertexFormat::UINT32x4:
            return 4;
        default:
            return 0;
        }
    }

    uint32_t ShaderReflector::FormatSizeBytes(VertexFormat format)
    {
        return ComponentCount(format) * sizeof(float); // float, int, uint all 4 bytes
    }

    const char* ShaderReflector::VertexFormatToString(VertexFormat format)
    {
        switch (format)
        {
        case VertexFormat::FLOAT32:     return "float";
        case VertexFormat::FLOAT32x2:   return "float2";
        case VertexFormat::FLOAT32x3:   return "float3";
        case VertexFormat::FLOAT32x4:   return "float4";
        case VertexFormat::INT32:       return "int";
        case VertexFormat::INT32x2:     return "int2";
        case VertexFormat::INT32x3:     return "int3";
        case VertexFormat::INT32x4:     return "int4";
        case VertexFormat::UINT32:      return "uint";
        case VertexFormat::UINT32x2:    return "uint2";
        case VertexFormat::UINT32x3:    return "uint3";
        case VertexFormat::UINT32x4:    return "uint4";
        default:                        return "unknown";
        }
    }

    const char* ShaderReflector::ResourceTypeToString(ResourceType type)
    {
        switch (type)
        {
        case ResourceType::UniformBuffer:  return "UniformBuffer";
        case ResourceType::StorageBuffer:  return "StorageBuffer";
        case ResourceType::SampledTexture: return "SampledTexture";
        case ResourceType::StorageTexture: return "StorageTexture";
        case ResourceType::Sampler:        return "Sampler";
        default:                           return "Unknown";
        }
    }

    const char* ShaderReflector::ShaderStageToString(ShaderStage stage)
    {
        switch (stage)
        {
        case ShaderStage::VERTEX:   return "Vertex";
        case ShaderStage::FRAGMENT: return "Fragment";
        case ShaderStage::COMPUTE:  return "Compute";
        default:                    return "Unknown";
        }
    }

} // namespace HBL2
