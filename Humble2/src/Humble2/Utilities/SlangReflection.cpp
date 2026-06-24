#include "SlangReflection.h"

#include "Renderer/Renderer.h"
#include "Resources/ResourceManager.h"

#include <algorithm>
#include <cassert>
#include <cstdio>

namespace HBL2
{
    static MemberBaseType ToMemberBaseType(slang::TypeReflection::ScalarType scalar)
    {
        switch (scalar)
        {
        case slang::TypeReflection::ScalarType::Bool:   return MemberBaseType::Bool;
        case slang::TypeReflection::ScalarType::Int32:  return MemberBaseType::Int;
        case slang::TypeReflection::ScalarType::UInt32: return MemberBaseType::UInt;
        case slang::TypeReflection::ScalarType::Float32:return MemberBaseType::Float;
        case slang::TypeReflection::ScalarType::Float64:return MemberBaseType::Double;
        default:                                        return MemberBaseType::Unknown;
        }
    }

    static MemberTypeInfo ReflectMemberType(slang::TypeLayoutReflection* fieldLayout)
    {
        MemberTypeInfo info;
        slang::TypeReflection* type = fieldLayout->getType();

        // Unwrap arrays first
        if (type->getKind() == slang::TypeReflection::Kind::Array)
        {
            info.isArray = true;
            info.arrayCount = static_cast<uint32_t>(type->getElementCount());
            fieldLayout = fieldLayout->getElementTypeLayout();
            type = fieldLayout->getType();
        }

        switch (type->getKind())
        {
        case slang::TypeReflection::Kind::Scalar:
            info.base = ToMemberBaseType(type->getScalarType());
            info.rows = 1;
            info.cols = 1;
            break;

        case slang::TypeReflection::Kind::Vector:
            info.base = ToMemberBaseType(type->getElementType()->getScalarType());
            info.rows = 1;
            info.cols = static_cast<uint32_t>(type->getElementCount()); // float3 → cols=3
            break;

        case slang::TypeReflection::Kind::Matrix:
            info.base = ToMemberBaseType(type->getElementType()->getScalarType());
            info.rows = static_cast<uint32_t>(type->getRowCount());
            info.cols = static_cast<uint32_t>(type->getColumnCount());
            break;

        default:
            break; // nested structs etc. — extend recursively if needed
        }

        return info;
    }

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

    Span<const Span<const ShaderConstant>> ShaderReflectionData::GetSpecializationConstantsPerVariant(Span<const ShaderDescriptor::RenderPipeline::PackedVariant> variants)
    {
        // Only bool spec constants participate in PSO selection.
        // specializationConstants is already sorted by constantId from ReflectSpecializationConstants,
        // so positional index here matches shaderConstantBool0..7 order.
        std::vector<const ReflectedSpecializationConstant*> boolConstants;
        for (const auto& sc : specializationConstants)
        {
            if (sc.type == ShaderConstantType::Bool)
            {
                boolConstants.push_back(&sc);
            }
        }

        HBL2_CORE_ASSERT(boolConstants.size() <= 8, "PackedVariant only supports up to 8 bool shader constants");

        m_SpecConstantStorage.clear();
        m_SpecConstantStorage.reserve(variants.Size() * boolConstants.size());
        m_SpecConstantsPerVariant.clear();
        m_SpecConstantsPerVariant.reserve(variants.Size());

        for (const auto& variant : variants)
        {
            const size_t startIndex = m_SpecConstantStorage.size();

            for (uint32_t i = 0; i < (uint32_t)boolConstants.size(); ++i)
            {
                bool extractedBool;

                switch (i)
                {
                case 0: extractedBool = (bool)variant.shaderConstantBool0; break;
                case 1: extractedBool = (bool)variant.shaderConstantBool1; break;
                case 2: extractedBool = (bool)variant.shaderConstantBool2; break;
                case 3: extractedBool = (bool)variant.shaderConstantBool3; break;
                case 4: extractedBool = (bool)variant.shaderConstantBool4; break;
                case 5: extractedBool = (bool)variant.shaderConstantBool5; break;
                case 6: extractedBool = (bool)variant.shaderConstantBool6; break;
                case 7: extractedBool = (bool)variant.shaderConstantBool7; break;
                }

                m_SpecConstantStorage.push_back(ShaderConstantBool(boolConstants[i]->stageMask, extractedBool));
            }

            m_SpecConstantsPerVariant.push_back({m_SpecConstantStorage.data() + startIndex, boolConstants.size() });
        }

        return { m_SpecConstantsPerVariant.data(), m_SpecConstantsPerVariant.size() };
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

    const ShaderDescriptor::RenderPipeline::VertexBufferBinding* ShaderReflectionData::findVertexBufferBinding(uint32_t bufferBinding) const
    {
        for (const auto& vbb : vertexBufferBindings)
        {
            if (vbb.bufferBinding == bufferBinding)
            {
                return &vbb;
            }
        }
        return nullptr;
    }

    const ReflectedSpecializationConstant* ShaderReflectionData::findSpecializationConstant(const std::string& name) const
    {
        for (const auto& sc : specializationConstants)
        {
            if (sc.name == name)
            {
                return &sc;
            }
        }
        return nullptr;
    }

    const ReflectedSpecializationConstant* ShaderReflectionData::findSpecializationConstantById(uint32_t constantId) const
    {
        for (const auto& sc : specializationConstants)
        {
            if (sc.constantId == constantId)
            {
                return &sc;
            }
        }
        return nullptr;
    }

    Handle<BindGroupLayout> ShaderReflectionData::GetBindGroupLayout(uint32_t set)
    {
        for (const auto& descriptorSet : descriptorSets)
        {
            if (descriptorSet.set != set)
            {
                continue;
            }

            StaticDArray<BindGroupLayoutDescriptor::TextureBinding, 8> textureBindings;
            StaticDArray<BindGroupLayoutDescriptor::BufferBinding, 8> bufferBindings;

            for (const auto& b : descriptorSet.bindings)
            {
                ShaderStage shaderStage = ShaderStage::NONE;

                if (b.stageMask.IsSet(ShaderStage::VERTEX))
                {
                    shaderStage = ShaderStage::VERTEX;
                }
                else if (b.stageMask.IsSet(ShaderStage::FRAGMENT))
                {
                    shaderStage = ShaderStage::FRAGMENT;
                }
                else if (b.stageMask.IsSet(ShaderStage::COMPUTE))
                {
                    shaderStage = ShaderStage::COMPUTE;
                }

                if (b.type == ResourceType::SampledTexture || b.type == ResourceType::StorageTexture)
                {
                    textureBindings.push_back({
                        .slot = b.binding,
                        .visibility = shaderStage,
                        .type = b.type == ResourceType::SampledTexture ? TextureBindingType::IMAGE_SAMPLER : TextureBindingType::STORAGE_IMAGE,
                    });
                }
                else if (b.type == ResourceType::UniformBuffer || b.type == ResourceType::StorageBuffer)
                {
                    bufferBindings.push_back({
                        .slot = b.binding,
                        .visibility = shaderStage,
                        .type = b.type == ResourceType::UniformBuffer ? BufferBindingType::UNIFORM : BufferBindingType::STORAGE,
                    });
                }
            }

            auto bindGroupLayout = ResourceManager::Instance->CreateBindGroupLayout({
                .debugName = "reflected-bind-group-layout",
                .textureBindings = { textureBindings.data(), textureBindings.size() },
                .bufferBindings = { bufferBindings.data(), bufferBindings.size() },
                .createdFromReflection = true,
            });

            return bindGroupLayout;
        }

        return Renderer::Instance->GetEmptyBindingsLayout();
    }

    bool ShaderReflectionData::hasBinding(const std::string& name) const
    {
        return findBinding(name) != nullptr;
    }

    void ShaderReflectionData::print() const
    {
        printf("=== Shader Reflection: %s ===\n\n", sourcePath.c_str());

        // Entry points
        printf("Entry Points (%zu):\n", entryPoints.size());
        for (const auto& ep : entryPoints)
        {
            printf("  [%s]  %s\n", ShaderReflector::ShaderStageToString(ep.stage), ep.name.c_str());
        }
        printf("\n");

        // Vertex attributes
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

        // Vertex buffer bindings
        printf("Vertex Buffer Bindings (%zu):\n", vertexBufferBindings.size());
        for (const auto& vbb : vertexBufferBindings)
        {
            printf("  Buffer binding=%u  stride=%u bytes\n", vbb.bufferBinding, vbb.byteStride);
            for (uint32_t i = 0; i < static_cast<uint32_t>(vbb.attributes.size()); ++i)
            {
                const auto& attr = vbb.attributes[i];
                printf("    [%u]  byteOffset=%-4u  %s\n",
                    i,
                    attr.byteOffset,
                    ShaderReflector::VertexFormatToString(attr.format));
            }
        }
        printf("\n");

        // Descriptor sets
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
                    ShaderReflector::ShaderStageToString(b.stageMask),
                    b.count);
            }
        }
        printf("\n");

        // Specialization Constants
        printf("Specialization Constants (%zu):\n", specializationConstants.size());
        for (const auto& sc : specializationConstants)
        {
            printf("  constant_id=%-2u  %-24s  %-8s  stage=%s\n",
                sc.constantId,
                sc.name.c_str(),
                ShaderReflector::ConstantTypeToString(sc.type));
        }
        printf("\n");
    }

    void ShaderReflectionData::Clear()
    {
        entryPoints.clear();
        vertexAttributes.clear();
        vertexBufferBindings.clear();
        descriptorSets.clear();
        specializationConstants.clear();

        m_SpecConstantStorage.clear();
        m_SpecConstantsPerVariant.clear();
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
        ReflectDescriptorSets(linkedProgram, layout, data);
        ReflectSpecializationConstants(linkedProgram, layout, data);

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
        // Find the vertex entry point
        slang::EntryPointReflection* vertexEP = nullptr;
        for (uint32_t i = 0; i < layout->getEntryPointCount(); ++i)
        {
            slang::EntryPointReflection* ep = layout->getEntryPointByIndex(i);
            if (ep->getStage() == SLANG_STAGE_VERTEX)
            {
                vertexEP = ep; break;
            }
        }

        if (!vertexEP)
        {
            return;
        }

        // Each top-level parameter of the vertex entry point maps to one vertex
        // buffer binding slot. Parameters that are structs get flattened into
        // individual VertexAttributes; scalar/vector parameters become one each.
        const uint32_t paramCount = static_cast<uint32_t>(vertexEP->getParameterCount());

        for (uint32_t i = 0; i < paramCount; ++i)
        {
            slang::VariableLayoutReflection* param = vertexEP->getParameterByIndex(i);
            slang::TypeLayoutReflection* typeLayout = param->getTypeLayout();
            slang::TypeReflection* type = typeLayout->getType();

            // Collect the attributes that belong to this buffer binding slot
            // into a temporary list so we can sort and compute offsets cleanly.
            std::vector<VertexAttribute> bindingAttribs;

            if (type->getKind() == slang::TypeReflection::Kind::Struct)
            {
                const uint32_t fieldCount = static_cast<uint32_t>(type->getFieldCount());
                for (uint32_t f = 0; f < fieldCount; ++f)
                {
                    slang::VariableReflection* field = type->getFieldByIndex(f);
                    slang::VariableLayoutReflection* fieldLayout = typeLayout->getFieldByIndex(f);

                    int32_t location = static_cast<int32_t>(fieldLayout->getOffset(SLANG_PARAMETER_CATEGORY_VARYING_INPUT));

                    if (location < 0 || static_cast<uint32_t>(location) == ~0u)
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

                    bindingAttribs.push_back(std::move(attr));
                }
            }
            else
            {
                // Bare scalar/vector input — treat the parameter itself as one attribute
                int32_t location = static_cast<int32_t>(param->getOffset(SLANG_PARAMETER_CATEGORY_VARYING_INPUT));

                if (location >= 0 && static_cast<uint32_t>(location) != ~0u)
                {
                    VertexFormat fmt = ToVertexFormat(type);

                    VertexAttribute attr;
                    attr.name = param->getName();
                    attr.location = static_cast<uint32_t>(location);
                    attr.format = fmt;
                    attr.componentCount = ComponentCount(fmt);
                    attr.sizeBytes = FormatSizeBytes(fmt);

                    bindingAttribs.push_back(std::move(attr));
                }
            }

            if (bindingAttribs.empty())
            {
                continue;
            }

            // Sort by location so byte offsets are computed in the right order
            std::sort(bindingAttribs.begin(), bindingAttribs.end(),
                [](const VertexAttribute& a, const VertexAttribute& b)
                {
                    return a.location < b.location;
                });

            // Build the VertexBufferBinding: compute per-attribute byte offsets
            // and accumulate the total stride.
            ShaderDescriptor::RenderPipeline::VertexBufferBinding vbb;
            vbb.bufferBinding = i;
            vbb.byteStride = 0;

            for (const auto& attr : bindingAttribs)
            {
                ShaderDescriptor::RenderPipeline::VertexBufferBinding::Attribute bindingAttr;
                bindingAttr.byteOffset = vbb.byteStride;
                bindingAttr.format = attr.format;

                vbb.attributes.push_back(bindingAttr);
                vbb.byteStride += attr.sizeBytes;
            }

            out.vertexBufferBindings.push_back(std::move(vbb));

            // Also append to the flat global list for location-based lookups
            for (auto& attr : bindingAttribs)
            {
                out.vertexAttributes.push_back(std::move(attr));
            }
        }

        // Keep the global flat list sorted by location
        std::sort(out.vertexAttributes.begin(), out.vertexAttributes.end(),
            [](const VertexAttribute& a, const VertexAttribute& b)
            {
                return a.location < b.location;
            });
    }

    void ShaderReflector::ReflectDescriptorSets(slang::IComponentType* linkedProgram, slang::ProgramLayout* layout, ShaderReflectionData& out)
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
            slang::TypeLayoutReflection* typeLayout = param->getTypeLayout();

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

            DescriptorBinding descBinding;

            uint64_t bindingSize = 0;
            if (resourceType == ResourceType::UniformBuffer || resourceType == ResourceType::StorageBuffer || resourceType == ResourceType::StorageBufferReadOnly)
            {
                // getElementTypeLayout() peels off the ConstantBuffer<> / StructuredBuffer<> wrapper to reach the inner T.
                slang::TypeLayoutReflection* elementLayout = typeLayout->getElementTypeLayout();
                if (elementLayout)
                {
                    bindingSize = static_cast<uint64_t>(elementLayout->getSize(SLANG_PARAMETER_CATEGORY_UNIFORM));

                    const uint32_t fieldCount = static_cast<uint32_t>(elementLayout->getFieldCount());
                    descBinding.members.reserve(fieldCount);

                    for (uint32_t f = 0; f < fieldCount; ++f)
                    {
                        slang::VariableLayoutReflection* field = elementLayout->getFieldByIndex(f);

                        UniformMember member;
                        member.name = field->getName();
                        member.offset = static_cast<uint32_t>(field->getOffset(SLANG_PARAMETER_CATEGORY_UNIFORM));
                        member.size = static_cast<uint32_t>(field->getTypeLayout()->getSize(SLANG_PARAMETER_CATEGORY_UNIFORM));
                        member.typeInfo = ReflectMemberType(field->getTypeLayout());

                        descBinding.members.push_back(std::move(member));
                    }
                }
            }

            // Determine which stage this binding is used in.
            ShaderStage usedInStage = ShaderStage::NONE;
            BitFlags<ShaderStage> stageMask = { ShaderStage::NONE };
            for (uint32_t ep = 0; ep < layout->getEntryPointCount(); ++ep)
            {
                Slang::ComPtr<slang::IMetadata> metadata;
                Slang::ComPtr<slang::IBlob> diagnostics;

                if (SLANG_FAILED(linkedProgram->getEntryPointMetadata(ep, 0, metadata.writeRef(), diagnostics.writeRef())))
                {
                    HBL2_CORE_ERROR("Slang: Failed to get entry point metadata");
                    continue;
                }

                SlangStage stage = layout->getEntryPointByIndex(ep)->getStage();

                bool used = false;
                if (SLANG_FAILED(metadata->isParameterLocationUsed((SlangParameterCategory)param->getCategory(), set, binding, used)))
                {
                    HBL2_CORE_ERROR("Slang: Failed to check if descriptor set is being used");
                    continue;
                }

                if (used)
                {
                    switch (stage)
                    {
                    case SLANG_STAGE_VERTEX:
                        stageMask.Set(ShaderStage::VERTEX);
                        break;
                    case SLANG_STAGE_FRAGMENT:
                        stageMask.Set(ShaderStage::FRAGMENT);
                        break;
                    case SLANG_STAGE_COMPUTE:
                        stageMask.Set(ShaderStage::COMPUTE);
                        break;
                    }
                }
            }

            descBinding.name = param->getName();
            descBinding.binding = binding;
            descBinding.set = set;
            descBinding.type = resourceType;
            descBinding.size = bindingSize;
            descBinding.count = arrayCount;
            descBinding.stageMask = stageMask;

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

    void ShaderReflector::ReflectSpecializationConstants(slang::IComponentType* linkedProgram, slang::ProgramLayout* layout, ShaderReflectionData& out)
    {
        slang::ProgramLayout* programLayout = linkedProgram->getLayout();

        for (SlangUInt i = 0; i < layout->getParameterCount(); ++i)
        {
            auto param = layout->getParameterByIndex(i);

            if (param->getCategory() == slang::ParameterCategory::SpecializationConstant)
            {
                printf("Specialization constant: %s\n", param->getName());

                ReflectedSpecializationConstant sc;
                sc.name = param->getName();
                sc.constantId = param->getBindingIndex();
                sc.type = ToConstantType(param->getType());

                // See where it's visible/used
                for (SlangUInt ep = 0; ep < layout->getEntryPointCount(); ++ep)
                {
                    Slang::ComPtr<slang::IMetadata> metadata;
                    Slang::ComPtr<slang::IBlob> diagnostics;

                    if (SLANG_FAILED(linkedProgram->getEntryPointMetadata(ep, 0, metadata.writeRef(), diagnostics.writeRef())))
                    {
                        HBL2_CORE_ERROR("Slang: Failed to get entry point metadata");
                        continue;
                    }

                    SlangStage stage = programLayout->getEntryPointByIndex(ep)->getStage();

                    bool used = false;
                    if (SLANG_FAILED(metadata->isParameterLocationUsed(SLANG_PARAMETER_CATEGORY_SPECIALIZATION_CONSTANT, 0, sc.constantId, used)))
                    {
                        HBL2_CORE_ERROR("Slang: Failed to check if spetialization constant is being used");
                        continue;
                    }

                    if (used)
                    {
                        switch (stage)
                        {
                        case SLANG_STAGE_VERTEX:
                            sc.stageMask.Set(ShaderStage::VERTEX);
                            break;
                        case SLANG_STAGE_FRAGMENT:
                            sc.stageMask.Set(ShaderStage::FRAGMENT);
                            break;
                        case SLANG_STAGE_COMPUTE:
                            sc.stageMask.Set(ShaderStage::COMPUTE);
                            break;
                        }
                    }
                }

                out.specializationConstants.push_back(std::move(sc));
            }
        }

        // Keep sorted by constant_id for deterministic output
        std::sort(out.specializationConstants.begin(), out.specializationConstants.end(),
            [](const ReflectedSpecializationConstant& a, const ReflectedSpecializationConstant& b)
            {
                return a.constantId < b.constantId;
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
                if (access == SLANG_RESOURCE_ACCESS_READ)
                {
                    return ResourceType::StorageBufferReadOnly;
                }

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

    const char* ShaderReflector::ShaderStageToString(BitFlags<ShaderStage> stageMask)
    {
        if (stageMask.IsSet(ShaderStage::VERTEX) && stageMask.IsSet(ShaderStage::FRAGMENT))
        {
            return "Vertex | Fragment";
        }

        if (stageMask.IsSet(ShaderStage::VERTEX))
        {
            return "Vertex";
        }

        if (stageMask.IsSet(ShaderStage::FRAGMENT))
        {
            return "Fragment";
        }

        if (stageMask.IsSet(ShaderStage::COMPUTE))
        {
            return "Compute";
        }

        return "Unknown";
    }

    ShaderConstantType ShaderReflector::ToConstantType(slang::TypeReflection* type)
    {
        switch (type->getScalarType())
        {
        case SLANG_SCALAR_TYPE_BOOL:    return ShaderConstantType::Bool;
        case SLANG_SCALAR_TYPE_INT32:   return ShaderConstantType::Int;
        case SLANG_SCALAR_TYPE_UINT32:  return ShaderConstantType::UInt;
        case SLANG_SCALAR_TYPE_FLOAT32: return ShaderConstantType::Float;
        default:                        return ShaderConstantType::Float; // best-effort fallback
        }
    }

    const char* ShaderReflector::ConstantTypeToString(ShaderConstantType type)
    {
        switch (type)
        {
        case ShaderConstantType::Bool:  return "bool";
        case ShaderConstantType::Int:   return "int";
        case ShaderConstantType::UInt:  return "uint";
        case ShaderConstantType::Float: return "float";
        default:                        return "unknown";
        }
    }

} // namespace HBL2
