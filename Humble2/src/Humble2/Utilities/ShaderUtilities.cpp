#include "ShaderUtilities.h"

#include "YamlUtilities.h"
#include "Collections/StaticDArray.h"

#include "Project\Project.h"

#include "slang-com-ptr.h"
#include "slang.h"

namespace HBL2
{
    ShaderUtilities* ShaderUtilities::s_Instance = nullptr;

    static Slang::ComPtr<slang::IGlobalSession>* g_SLangGlobalSessions;

    static bool VariantExists(const YAML::Node& existingVariants, uint64_t newVariantHash)
    {
        for (const auto& variant : existingVariants)
        {
            if (variant["Variant"].as<uint64_t>() == newVariantHash)
            {
                return true;
            }
        }

        return false;
    }

    static YAML::Node VariantToYAMLNode(uint64_t newVariantHash, const ShaderDescriptor::RenderPipeline::PackedVariant& variant)
    {
        YAML::Node baseVariant;

        baseVariant["Variant"] = newVariantHash;

        baseVariant["RasterState"]["Topology"] = (int)variant.topology;
        baseVariant["RasterState"]["PolygonMode"] = (int)variant.polygonMode;
        baseVariant["RasterState"]["CullMode"] = (int)variant.cullMode;
        baseVariant["RasterState"]["FrontFace"] = (int)variant.frontFace;

        baseVariant["BlendState"]["Enabled"] = (bool)variant.blendEnabled;
        baseVariant["BlendState"]["ColorOutputEnabled"] = (bool)variant.colorOutput;

        baseVariant["DepthState"]["Enabled"] = (bool)variant.depthEnabled;
        baseVariant["DepthState"]["WriteEnabled"] = (bool)variant.depthWrite;
        baseVariant["DepthState"]["StencilEnabled"] = (bool)variant.stencilEnabled;
        baseVariant["DepthState"]["DepthTest"] = (int)variant.depthCompare;

        baseVariant["ShaderConstantsState"]["ShaderConstantBool0"] = (bool)variant.shaderConstantBool0;
        baseVariant["ShaderConstantsState"]["ShaderConstantBool1"] = (bool)variant.shaderConstantBool1;
        baseVariant["ShaderConstantsState"]["ShaderConstantBool2"] = (bool)variant.shaderConstantBool2;
        baseVariant["ShaderConstantsState"]["ShaderConstantBool3"] = (bool)variant.shaderConstantBool3;
        baseVariant["ShaderConstantsState"]["ShaderConstantBool4"] = (bool)variant.shaderConstantBool4;
        baseVariant["ShaderConstantsState"]["ShaderConstantBool5"] = (bool)variant.shaderConstantBool5;
        baseVariant["ShaderConstantsState"]["ShaderConstantBool6"] = (bool)variant.shaderConstantBool6;
        baseVariant["ShaderConstantsState"]["ShaderConstantBool7"] = (bool)variant.shaderConstantBool7;

        return baseVariant;
    }

    ShaderUtilities& ShaderUtilities::Get()
    {
        HBL2_CORE_ASSERT(s_Instance != nullptr, "ShaderUtilities::s_Instance is null! Call ShaderUtilities::Initialize before use.");
        return *s_Instance;
    }

    void ShaderUtilities::Initialize()
    {
        HBL2_CORE_ASSERT(s_Instance == nullptr, "ShaderUtilities::s_Instance is not null! ShaderUtilities::Initialize has been called twice.");
        s_Instance = new ShaderUtilities;
    }

    void ShaderUtilities::Shutdown()
    {
        HBL2_CORE_ASSERT(s_Instance != nullptr, "ShaderUtilities::s_Instance is null!");

        delete s_Instance;
        s_Instance = nullptr;
    }

    ShaderUtilities::ShaderUtilities()
    {
        m_Reservation = Allocator::Arena.Reserve("ShaderUtilitiesPool", 16_MB);
        m_Arena.Initialize(&Allocator::Arena, 16_MB, m_Reservation);

        m_ShaderAssets = MakeDArray<Handle<Asset>>(m_Arena, 1024);
        m_Shaders = MakeHMap<BuiltInShader, Handle<Shader>>(m_Arena, 1024);
        // m_ShaderLayouts = MakeHMap<BuiltInShader, Handle<BindGroupLayout>>(m_Arena, 64);

        uint32_t globalSessionNum = std::thread::hardware_concurrency();

        g_SLangGlobalSessions = (Slang::ComPtr<slang::IGlobalSession>*)m_Arena.Alloc(globalSessionNum * sizeof(Slang::ComPtr<slang::IGlobalSession>));
        g_SLangGlobalSessions = m_Arena.ConstructArray<Slang::ComPtr<slang::IGlobalSession>>(g_SLangGlobalSessions, globalSessionNum);

        {
            HBL2_PROFILE("SLang Global Sessions Creation");

            JobContext ctx;
            JobSystem::Get().Dispatch(ctx, globalSessionNum, 1, [](JobDispatchArgs args)
            {
                slang::createGlobalSession(g_SLangGlobalSessions[args.jobIndex].writeRef());
            });
            JobSystem::Get().Wait(ctx);
        }
    }    

    std::string ShaderUtilities::ReadFile(const std::string& filepath)
    {
        std::string result;
        std::ifstream in(filepath, std::ios::in | std::ios::binary);

        if (in)
        {
            in.seekg(0, std::ios::end);
            size_t size = in.tellg();
            if (size != -1)
            {
                result.resize(size);
                in.seekg(0, std::ios::beg);
                in.read(&result[0], size);
            }
            else
            {
                HBL2_CORE_ERROR("Could not read from file '{0}'", filepath);
            }
        }
        else
        {
            HBL2_CORE_ERROR("Could not open file '{0}'", filepath);
        }

        return result;
    }

    CompilationResultData ShaderUtilities::Compile(const std::string& shaderFilePath, ShaderReflectionData* outReflectionData, bool forceRecompile)
    {
        HBL2_FUNC_PROFILE();

        // https://docs.shader-slang.org/en/stable/coming-from-glsl.html

        GraphicsAPI target = Renderer::Instance->GetAPI();
        CreateCacheDirectoryIfNeeded(target);

        std::filesystem::path shaderPath = shaderFilePath;
        std::filesystem::path cacheDirectory = GetCacheDirectory(target);

        uint32_t workerIndex = JobSystem::Get().GetWorkerIndex();

        // Target description.
        // NOTE: No GENERATE_WHOLE_PROGRAM — we want one SPIR-V binary per entry point.
        slang::TargetDesc targetDesc = {};
        targetDesc.format = SLANG_SPIRV; // https://docs.shader-slang.org/en/latest/external/slang/docs/user-guide/a2-01-spirv-target-specific.html
        targetDesc.flags = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;

        switch (target)
        {
        case GraphicsAPI::OPENGL:
            targetDesc.profile = g_SLangGlobalSessions[workerIndex]->findProfile("glsl_460");
            break;
        case GraphicsAPI::VULKAN:
            targetDesc.profile = g_SLangGlobalSessions[workerIndex]->findProfile("spirv_1_3");
            break;
        }

        // Session description.
        slang::SessionDesc sessionDesc = {};
        sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

        // https://docs.shader-slang.org/en/latest/external/slang/docs/user-guide/08-compiling.html

        std::array<slang::CompilerOptionEntry, 4> options =
        {
            slang::CompilerOptionEntry
            {
                .name = slang::CompilerOptionName::EmitSpirvDirectly,
                .value = { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1 },
            },
            slang::CompilerOptionEntry
            {
                .name = slang::CompilerOptionName::VulkanUseEntryPointName,
                .value = { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1 },
            },
            slang::CompilerOptionEntry
            {
                .name = slang::CompilerOptionName::MatrixLayoutColumn,
                .value = { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1 }
            },
            slang::CompilerOptionEntry
            {
                .name = slang::CompilerOptionName::Optimization,
                .value = { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = SlangOptimizationLevel::SLANG_OPTIMIZATION_LEVEL_NONE }
            }
        };

        sessionDesc.compilerOptionEntries = options.data();
        sessionDesc.compilerOptionEntryCount = options.size();

        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;

        Slang::ComPtr<slang::ISession> session;
        if (SLANG_FAILED(g_SLangGlobalSessions[workerIndex]->createSession(sessionDesc, session.writeRef())))
        {
            HBL2_CORE_ERROR("Slang: Failed to create session");
            return {};
        }

        // Module.
        std::string shaderSource = ReadFile(shaderFilePath);

        Slang::ComPtr<slang::IBlob> diagnostics;
        Slang::ComPtr<slang::IModule> slangModule;
        slangModule = session->loadModuleFromSourceString(shaderPath.filename().stem().string().c_str(), shaderFilePath.c_str(), shaderSource.c_str(), diagnostics.writeRef());

        if (diagnostics)
        {
            HBL2_CORE_WARN("Slang diagnostics: {}", (const char*)diagnostics->getBufferPointer());
            diagnostics = nullptr;
        }

        if (!slangModule)
        {
            HBL2_CORE_ERROR("Slang: Failed to load shader module");
            return {};
        }

        // Entry points.
        SlangInt32 entryPointCount = slangModule->getDefinedEntryPointCount();
        if (entryPointCount == 0)
        {
            HBL2_CORE_ERROR("Slang: No entry points found in shader source");
            return {};
        }

        StaticDArray<Slang::ComPtr<slang::IEntryPoint>, 6> entryPoints;
        for (SlangInt32 i = 0; i < entryPointCount; ++i)
        {
            Slang::ComPtr<slang::IEntryPoint> ep;
            slangModule->getDefinedEntryPoint(i, ep.writeRef());
            entryPoints.push_back(std::move(ep));
        }

        // Composite + link.
        StaticDArray<slang::IComponentType*, 6> components;
        components.push_back(slangModule);
        for (auto& ep : entryPoints)
        {
            components.push_back(ep.get());
        }

        Slang::ComPtr<slang::IComponentType> linkedProgram;
        if (SLANG_FAILED(session->createCompositeComponentType(components.data(), (SlangInt)components.size(), linkedProgram.writeRef(), diagnostics.writeRef())))
        {
            if (diagnostics)
            {
                HBL2_CORE_ERROR("Slang link error: {}", (const char*)diagnostics->getBufferPointer());
            }

            HBL2_CORE_ERROR("Slang: Failed to link shader program");
            return {};
        }

        // Compile — one SPIR-V binary per entry point, each in its own vector.
        CompilationResultData compilationResultData;

        for (SlangInt i = 0; i < (SlangInt)entryPoints.size(); ++i)
        {
            const char* shaderStageName = nullptr;

            if (IsVertexStage(i, entryPointCount))
            {
                shaderStageName = "vert";
            }
            else if (IsFragmentStage(i, entryPointCount))
            {
                shaderStageName = "frag";
            }
            else if (IsComputeStage(i, entryPointCount))
            {
                shaderStageName = "comp";
            }

            if (shaderStageName == nullptr)
            {
                HBL2_CORE_WARN("Slang: Unknown entry point name");
                continue;
            }

            // Cache path per entry point.
            std::filesystem::path cachedPath = cacheDirectory / (shaderPath.filename().string() + ".cached." + shaderStageName + ".spv");

            // Cache hit for this entry point.
            if (!forceRecompile)
            {
                std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
                if (in.is_open())
                {
                    in.seekg(0, std::ios::end);
                    auto size = in.tellg();
                    in.seekg(0, std::ios::beg);

                    CompilationResultData::ShaderCode* shaderCode = nullptr;

                    if (IsVertexStage(i, entryPointCount))
                    {
                        compilationResultData.vertexShaderCode.ptr = (uint32_t*)JobSystem::Get().GetWorkerArena()->Alloc(size, alignof(uint32_t));
                        compilationResultData.vertexShaderCode.size = size / sizeof(uint32_t);

                        shaderCode = &compilationResultData.vertexShaderCode;
                    }
                    else if (IsFragmentStage(i, entryPointCount))
                    {
                        compilationResultData.fragmentShaderCode.ptr = (uint32_t*)JobSystem::Get().GetWorkerArena()->Alloc(size, alignof(uint32_t));
                        compilationResultData.fragmentShaderCode.size = size / sizeof(uint32_t);

                        shaderCode = &compilationResultData.fragmentShaderCode;
                    }
                    else if (IsComputeStage(i, entryPointCount))
                    {
                        compilationResultData.computeShaderCode.ptr = (uint32_t*)JobSystem::Get().GetWorkerArena()->Alloc(size, alignof(uint32_t));
                        compilationResultData.computeShaderCode.size = size / sizeof(uint32_t);

                        shaderCode = &compilationResultData.computeShaderCode;
                    }

                    in.read(reinterpret_cast<char*>(shaderCode->ptr), size);
                    continue;
                }
            }

            // Compile.
            Slang::ComPtr<slang::IBlob> code;
            diagnostics = nullptr;

            if (SLANG_FAILED(linkedProgram->getEntryPointCode(i, 0, code.writeRef(), diagnostics.writeRef())))
            {
                if (diagnostics)
                {
                    HBL2_CORE_ERROR("Slang compile error: {}", (const char*)diagnostics->getBufferPointer());
                }

                HBL2_CORE_ERROR("Slang: Failed to get SPIR-V for entry point {}", i);
                return {};
            }

            if (diagnostics)
            {
                HBL2_CORE_WARN("Slang: {}", (const char*)diagnostics->getBufferPointer());
            }

            const uint32_t* spirvData = static_cast<const uint32_t*>(code->getBufferPointer());
            size_t spirvByteSize = code->getBufferSize();
            size_t spirvSize = spirvByteSize / sizeof(uint32_t);

            CompilationResultData::ShaderCode* shaderCode = nullptr;

            if (IsVertexStage(i, entryPointCount))
            {
                compilationResultData.vertexShaderCode.ptr = (uint32_t*)JobSystem::Get().GetWorkerArena()->Alloc(spirvByteSize, alignof(uint32_t));
                compilationResultData.vertexShaderCode.size = spirvSize;

                shaderCode = &compilationResultData.vertexShaderCode;
            }
            else if (IsFragmentStage(i, entryPointCount))
            {
                compilationResultData.fragmentShaderCode.ptr = (uint32_t*)JobSystem::Get().GetWorkerArena()->Alloc(spirvByteSize, alignof(uint32_t));
                compilationResultData.fragmentShaderCode.size = spirvSize;

                shaderCode = &compilationResultData.fragmentShaderCode;
            }
            else if (IsComputeStage(i, entryPointCount))
            {
                compilationResultData.computeShaderCode.ptr = (uint32_t*)JobSystem::Get().GetWorkerArena()->Alloc(spirvByteSize, alignof(uint32_t));
                compilationResultData.computeShaderCode.size = spirvSize;

                shaderCode = &compilationResultData.computeShaderCode;
            }

            if (shaderCode == nullptr)
            {
                HBL2_CORE_WARN("Slang: Unknown entry point");
                continue;
            }

            std::memcpy(shaderCode->ptr, spirvData, spirvByteSize);

            // Write cache.
            std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
            if (out.is_open())
            {
                out.write(reinterpret_cast<const char*>(shaderCode->ptr), shaderCode->size * sizeof(uint32_t));
                out.flush();
            }
        }

        if (!compilationResultData.IsValid())
        {
            HBL2_CORE_ERROR("Slang: Shader compiled to empty SPIR-V");
            return {};
        }

        // Reflection.
        if (outReflectionData != nullptr)
        {
            *outReflectionData = ShaderReflector::Reflect(linkedProgram, shaderFilePath);
        }

        return compilationResultData;
    }

    ShaderReflectionData ShaderUtilities::Reflect(const std::string& shaderFilePath)
    {
        HBL2_FUNC_PROFILE();

        // https://docs.shader-slang.org/en/stable/coming-from-glsl.html

        GraphicsAPI target = Renderer::Instance->GetAPI();
        CreateCacheDirectoryIfNeeded(target);

        std::filesystem::path shaderPath = shaderFilePath;
        std::filesystem::path cacheDirectory = GetCacheDirectory(target);

        uint32_t workerIndex = JobSystem::Get().GetWorkerIndex();

        // Target description.
        // NOTE: No GENERATE_WHOLE_PROGRAM — we want one SPIR-V binary per entry point.
        slang::TargetDesc targetDesc = {};
        targetDesc.format = SLANG_SPIRV; // https://docs.shader-slang.org/en/latest/external/slang/docs/user-guide/a2-01-spirv-target-specific.html
        targetDesc.flags = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;

        switch (target)
        {
        case GraphicsAPI::OPENGL:
            targetDesc.profile = g_SLangGlobalSessions[workerIndex]->findProfile("glsl_460");
            break;
        case GraphicsAPI::VULKAN:
            targetDesc.profile = g_SLangGlobalSessions[workerIndex]->findProfile("spirv_1_3");
            break;
        }

        // Session description.
        slang::SessionDesc sessionDesc = {};
        sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

        // https://docs.shader-slang.org/en/latest/external/slang/docs/user-guide/08-compiling.html

        std::array<slang::CompilerOptionEntry, 4> options =
        {
            slang::CompilerOptionEntry
            {
                .name = slang::CompilerOptionName::EmitSpirvDirectly,
                .value = {.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1 },
            },
            slang::CompilerOptionEntry
            {
                .name = slang::CompilerOptionName::VulkanUseEntryPointName,
                .value = {.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1 },
            },
            slang::CompilerOptionEntry
            {
                .name = slang::CompilerOptionName::MatrixLayoutColumn,
                .value = {.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1 }
            },
            slang::CompilerOptionEntry
            {
                .name = slang::CompilerOptionName::Optimization,
                .value = {.kind = slang::CompilerOptionValueKind::Int, .intValue0 = SlangOptimizationLevel::SLANG_OPTIMIZATION_LEVEL_NONE }
            }
        };

        sessionDesc.compilerOptionEntries = options.data();
        sessionDesc.compilerOptionEntryCount = options.size();

        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;

        Slang::ComPtr<slang::ISession> session;
        if (SLANG_FAILED(g_SLangGlobalSessions[workerIndex]->createSession(sessionDesc, session.writeRef())))
        {
            HBL2_CORE_ERROR("Slang: Failed to create session");
            return {};
        }

        // Module.
        std::string shaderSource = ReadFile(shaderFilePath);

        Slang::ComPtr<slang::IBlob> diagnostics;
        Slang::ComPtr<slang::IModule> slangModule;
        slangModule = session->loadModuleFromSourceString(shaderPath.filename().stem().string().c_str(), shaderFilePath.c_str(), shaderSource.c_str(), diagnostics.writeRef());

        if (diagnostics)
        {
            HBL2_CORE_WARN("Slang diagnostics: {}", (const char*)diagnostics->getBufferPointer());
            diagnostics = nullptr;
        }

        if (!slangModule)
        {
            HBL2_CORE_ERROR("Slang: Failed to load shader module");
            return {};
        }

        // Entry points.
        SlangInt32 entryPointCount = slangModule->getDefinedEntryPointCount();
        if (entryPointCount == 0)
        {
            HBL2_CORE_ERROR("Slang: No entry points found in shader source");
            return {};
        }

        StaticDArray<Slang::ComPtr<slang::IEntryPoint>, 6> entryPoints;
        for (SlangInt32 i = 0; i < entryPointCount; ++i)
        {
            Slang::ComPtr<slang::IEntryPoint> ep;
            slangModule->getDefinedEntryPoint(i, ep.writeRef());
            entryPoints.push_back(std::move(ep));
        }

        // Composite + link.
        StaticDArray<slang::IComponentType*, 6> components;
        components.push_back(slangModule);
        for (auto& ep : entryPoints)
        {
            components.push_back(ep.get());
        }

        Slang::ComPtr<slang::IComponentType> linkedProgram;
        if (SLANG_FAILED(session->createCompositeComponentType(components.data(), (SlangInt)components.size(), linkedProgram.writeRef(), diagnostics.writeRef())))
        {
            if (diagnostics)
            {
                HBL2_CORE_ERROR("Slang link error: {}", (const char*)diagnostics->getBufferPointer());
            }

            HBL2_CORE_ERROR("Slang: Failed to link shader program");
            return {};
        }

        return ShaderReflector::Reflect(linkedProgram, shaderFilePath);
    }

    void ShaderUtilities::LoadBuiltInShaders()
    {
        JobContext ctx;

        // Invalid shader
        auto invalidShaderAssetHandle = AssetManager::Instance->CreateAsset({
            .debugName = "invalid-shader-asset",
            .filePath = "assets/shaders/invalid.slang",
            .type = AssetType::Shader,
        });

        CreateShaderMetadataFile(invalidShaderAssetHandle, 0);
        auto* invalidShaderTask = AssetManager::Instance->GetAssetAsync<Shader>(invalidShaderAssetHandle, &ctx);

        // Unlit shader
        auto unlitShaderAssetHandle = AssetManager::Instance->CreateAsset({
            .debugName = "unlit-shader-asset",
            .filePath = "assets/shaders/unlit.slang",
            .type = AssetType::Shader,
        });

        CreateShaderMetadataFile(unlitShaderAssetHandle, 0);
        auto* unlitShaderTask = AssetManager::Instance->GetAssetAsync<Shader>(unlitShaderAssetHandle, &ctx);

        // Blinn-Phong shader
        auto blinnPhongShaderAssetHandle = AssetManager::Instance->CreateAsset({
            .debugName = "blinn-phong-shader-asset",
            .filePath = "assets/shaders/blinn-phong.slang",
            .type = AssetType::Shader,
        });

        CreateShaderMetadataFile(blinnPhongShaderAssetHandle, 1);
        auto* blinnPhongShaderTask = AssetManager::Instance->GetAssetAsync<Shader>(blinnPhongShaderAssetHandle, &ctx);

        // PBR shader
        auto pbrShaderAssetHandle = AssetManager::Instance->CreateAsset({
            .debugName = "pbr-shader-asset",
            .filePath = "assets/shaders/pbr.slang",
            .type = AssetType::Shader,
        });

        CreateShaderMetadataFile(pbrShaderAssetHandle, 1);
        auto* pbrShaderTask = AssetManager::Instance->GetAssetAsync<Shader>(pbrShaderAssetHandle, &ctx);

        m_ShaderAssets.push_back(invalidShaderAssetHandle);
        m_ShaderAssets.push_back(unlitShaderAssetHandle);
        m_ShaderAssets.push_back(blinnPhongShaderAssetHandle);
        m_ShaderAssets.push_back(pbrShaderAssetHandle);

        AssetManager::Instance->WaitForAsyncJobs(&ctx);

        m_Shaders[BuiltInShader::INVALID] = invalidShaderTask ? invalidShaderTask->ResourceHandle : Handle<Shader>();
        m_Shaders[BuiltInShader::UNLIT] = unlitShaderTask ? unlitShaderTask->ResourceHandle : Handle<Shader>();
        m_Shaders[BuiltInShader::BLINN_PHONG] = blinnPhongShaderTask ? blinnPhongShaderTask->ResourceHandle : Handle<Shader>();
        m_Shaders[BuiltInShader::PBR] = pbrShaderTask ? pbrShaderTask->ResourceHandle : Handle<Shader>();

        AssetManager::Instance->ReleaseResourceTask(invalidShaderTask);
        AssetManager::Instance->ReleaseResourceTask(unlitShaderTask);
        AssetManager::Instance->ReleaseResourceTask(blinnPhongShaderTask);
        AssetManager::Instance->ReleaseResourceTask(pbrShaderTask);
    }

    void ShaderUtilities::DeleteBuiltInShaders()
    {
        for (auto& [shaderType, shaderHandle] : m_Shaders)
        {
            ResourceManager::Instance->DeleteShader(shaderHandle);
        }

        m_Shaders.clear();
        m_ShaderAssets.clear();
    }

    void ShaderUtilities::LoadBuiltInMaterials()
    {
        LitMaterialAsset = AssetManager::Instance->CreateAsset({
            .debugName = "lit-material-asset",
            .filePath = "assets/materials/lit.mat",
            .type = AssetType::Material,
        });

        CreateMaterialMetadataFile(LitMaterialAsset, 1);

        AssetManager::Instance->GetAsset<Material>(LitMaterialAsset);
    }

    void ShaderUtilities::DeleteBuiltInMaterials()
    {
        AssetManager::Instance->DeleteAsset(LitMaterialAsset);
    }

    void ShaderUtilities::CreateShaderMetadataFile(Handle<Asset> handle, uint32_t shaderType)
    {
        Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);

        const auto& filesystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
        const auto& path = std::filesystem::exists(filesystemPath) ? filesystemPath : asset->FilePath;

        if (std::filesystem::exists(path.string() + ".hblshader"))
        {
            return;
        }

        if (!std::filesystem::exists(path.parent_path()))
        {
            try
            {
                std::filesystem::create_directories(path.parent_path());
            }
            catch (std::exception& e)
            {
                HBL2_ERROR("Project directory creation failed: {0}", e.what());
            }
        }

        std::ofstream fout(path.string() + ".hblshader", 0);

        ShaderDescriptor::RenderPipeline::PackedVariant variantHash = {};

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Shader" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "UUID" << YAML::Value << asset->UUID;
        out << YAML::Key << "Type" << YAML::Value << shaderType;

        out << YAML::Key << "BindGroup" << YAML::BeginSeq;
        out << YAML::BeginMap;

        const auto& reflectionData = Reflect(path.string());

        for (const auto& descriptorSet : reflectionData.descriptorSets)
        {
            if (descriptorSet.set != 2)
            {
                continue;
            }

            uint32_t bufferIndex = 0;
            uint32_t textureIndex = 0;

            for (const auto& b : descriptorSet.bindings)
            {
                if (b.type == ResourceType::UniformBuffer)
                {
                    out << YAML::Key << b.name.c_str();
                    out << YAML::BeginMap;

                    for (const auto& m : b.members)
                    {
                        switch (m.typeInfo.base)
                        {
                        case MemberBaseType::Float:
                            {
                                if (m.typeInfo.cols == 1)
                                {
                                    out << YAML::Key << m.name.c_str() << YAML::Value << 0.0f;
                                }
                                else if (m.typeInfo.cols == 2)
                                {
                                    out << YAML::Key << m.name.c_str() << YAML::Value << glm::vec2(0.0f);
                                }
                                else if (m.typeInfo.cols == 3)
                                {
                                    out << YAML::Key << m.name.c_str() << YAML::Value << glm::vec3(0.0f);
                                }
                                else if (m.typeInfo.cols == 4)
                                {
                                    out << YAML::Key << m.name.c_str() << YAML::Value << glm::vec4(0.0f);
                                }
                                break;
                            }
                        }
                    }

                    out << YAML::EndMap;
                }
                else if (b.type == ResourceType::SampledTexture)
                {
                    out << YAML::Key << b.name.c_str() << YAML::Value << (UUID)0;
                }
            }
        }
        out << YAML::EndMap;
        out << YAML::EndSeq;

        out << YAML::Key << "Variants" << YAML::BeginSeq;
        out << YAML::BeginMap;
        out << YAML::Key << "Variant" << YAML::Value << variantHash.Key();

        out << YAML::Key << "RasterState";
        out << YAML::BeginMap;
        out << YAML::Key << "Topology" << YAML::Value << (int)Topology::TRIANGLE_LIST;
        out << YAML::Key << "PolygonMode" << YAML::Value << (int)PolygonMode::FILL;
        out << YAML::Key << "CullMode" << YAML::Value << (int)CullMode::BACK;
        out << YAML::Key << "FrontFace" << YAML::Value << (int)FrontFace::CLOCKWISE;
        out << YAML::EndMap;

        out << YAML::Key << "BlendState";
        out << YAML::BeginMap;
        out << YAML::Key << "Enabled" << YAML::Value << false;
        out << YAML::Key << "ColorOutputEnabled" << YAML::Value << true;
        out << YAML::EndMap;

        out << YAML::Key << "DepthState";
        out << YAML::BeginMap;
        out << YAML::Key << "Enabled" << YAML::Value << true;
        out << YAML::Key << "WriteEnabled" << YAML::Value << false;
        out << YAML::Key << "StencilEnabled" << YAML::Value << true;
        out << YAML::Key << "DepthTest" << YAML::Value << (int)Compare::LESS_OR_EQUAL;
        out << YAML::EndMap;

        out << YAML::Key << "ShaderConstantsState";
        out << YAML::BeginMap;
        out << YAML::Key << "ShaderConstantBool0" << YAML::Value << false;
        out << YAML::Key << "ShaderConstantBool1" << YAML::Value << false;
        out << YAML::Key << "ShaderConstantBool2" << YAML::Value << false;
        out << YAML::Key << "ShaderConstantBool3" << YAML::Value << false;
        out << YAML::Key << "ShaderConstantBool4" << YAML::Value << false;
        out << YAML::Key << "ShaderConstantBool5" << YAML::Value << false;
        out << YAML::Key << "ShaderConstantBool6" << YAML::Value << false;
        out << YAML::Key << "ShaderConstantBool7" << YAML::Value << false;
        out << YAML::EndMap;

        out << YAML::EndMap;
        out << YAML::EndSeq;

        out << YAML::EndMap;
        out << YAML::EndMap;

        fout << out.c_str();
        fout.close();
    }

    void ShaderUtilities::UpdateShaderBindGroupResourcesAssetFile(Handle<Asset> handle, const ShaderDataDescriptor&& desc)
    {
        Asset* shaderAsset = AssetManager::Instance->GetAssetMetadata(handle);

        if (shaderAsset == nullptr)
        {
            return;
        }

        Handle<Shader> shaderHandle = AssetManager::Instance->GetAsset<Shader>(handle);

        const auto& filesystemPath = Project::GetAssetFileSystemPath(shaderAsset->FilePath);
        const auto& filePath = std::filesystem::exists(filesystemPath) ? filesystemPath : shaderAsset->FilePath;

        YAML::Node root = YAML::LoadFile(filePath.string() + ".hblshader");

        // Clear previous entries.
        root["Shader"]["BindGroup"] = YAML::Node(YAML::NodeType::Sequence);
        YAML::Node bindGroup = root["Shader"]["BindGroup"];

        // Get reflection data.
        const auto& reflectionData = Reflect(filePath.string());

        for (const auto& descriptorSet : reflectionData.descriptorSets)
        {
            if (descriptorSet.set != 1)
            {
                continue;
            }

            uint32_t bufferIndex = 0;
            uint32_t textureIndex = 0;

            const std::vector<uint8_t>& uniformBufferBytes = desc.Buffers[bufferIndex++];

            for (const auto& b : descriptorSet.bindings)
            {
                YAML::Node out;

                if (b.type == ResourceType::UniformBuffer)
                {
                    for (const auto& m : b.members)
                    {
                        const uint8_t* memberPtr = uniformBufferBytes.data() + m.offset;

                        switch (m.typeInfo.base)
                        {
                        case MemberBaseType::Float:
                            {
                                const float* f = reinterpret_cast<const float*>(memberPtr);

                                if (m.typeInfo.cols == 1)
                                {
                                    out[b.name.c_str()][m.name.c_str()] = *f;
                                }
                                else if (m.typeInfo.cols == 2)
                                {
                                    out[b.name.c_str()][m.name.c_str()] = glm::vec2(f[0], f[1]);
                                }
                                else if (m.typeInfo.cols == 3)
                                {
                                    out[b.name.c_str()][m.name.c_str()] = glm::vec3(f[0], f[1], f[2]);
                                }
                                else if (m.typeInfo.cols == 4)
                                {
                                    out[b.name.c_str()][m.name.c_str()] = glm::vec4(f[0], f[1], f[2], f[3]);
                                }
                                break;
                            }
                        }
                    }
                }
                else if (b.type == ResourceType::SampledTexture)
                {
                    Handle<Asset> textureAssetHandle = Handle<Asset>::UnPack(desc.TextureAssets[textureIndex++]);

                    if (textureAssetHandle.IsValid())
                    {
                        Asset* asset = AssetManager::Instance->GetAssetMetadata(textureAssetHandle);
                        out[b.name.c_str()] = asset->UUID;
                    }
                    else
                    {
                        out[b.name.c_str()] = (UUID)0;
                    }

                }

                bindGroup.push_back(out);
            }
        }

        std::ofstream fout(filePath.string() + ".hblshader");
        fout << root;
        fout.close();
    }

    void ShaderUtilities::UpdateShaderVariantMetadataFile(UUID shaderUUID, const ShaderDescriptor::RenderPipeline::PackedVariant& newVariant)
    {
        Handle<Asset> shaderAssetHandle = AssetManager::Instance->GetHandleFromUUID(shaderUUID);
        Asset* shaderAsset = AssetManager::Instance->GetAssetMetadata(shaderAssetHandle);

        if (shaderAsset == nullptr)
        {
            return;
        }

        Handle<Shader> shaderHandle = AssetManager::Instance->GetAsset<Shader>(shaderAssetHandle);

        const auto& filesystemPath = Project::GetAssetFileSystemPath(shaderAsset->FilePath);
        const auto& filePath = std::filesystem::exists(filesystemPath) ? filesystemPath : shaderAsset->FilePath;

        YAML::Node root = YAML::LoadFile(filePath.string() + ".hblshader");
        YAML::Node variants = root["Shader"]["Variants"];

        uint64_t newVariantHash = newVariant.Key();

        if (!VariantExists(variants, newVariantHash))
        {
            HBL2_CORE_INFO("New variant added.");
            variants.push_back(VariantToYAMLNode(newVariantHash, newVariant));
            std::ofstream fout(filePath.string() + ".hblshader");
            fout << root;
            fout.close();
        }
        else
        {
            HBL2_CORE_INFO("Variant already exists.");
        }
    }

    void ShaderUtilities::CreateMaterialMetadataFile(Handle<Asset> handle, uint32_t materialType, bool autoImported)
    {
        Asset* asset = AssetManager::Instance->GetAssetMetadata(handle);

        const auto& filesystemPath = Project::GetAssetFileSystemPath(asset->FilePath);
        const auto& path = std::filesystem::exists(filesystemPath) ? filesystemPath : asset->FilePath;

        if (std::filesystem::exists(path.string() + ".hblmat"))
        {
            return;
        }

        if (!std::filesystem::exists(path.parent_path()))
        {
            try
            {
                std::filesystem::create_directories(path.parent_path());
            }
            catch (std::exception& e)
            {
                HBL2_ERROR("Project directory creation failed: {0}", e.what());
            }
        }

        std::ofstream fout(path.string() + ".hblmat", 0);

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Material" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "UUID" << YAML::Value << asset->UUID;
        out << YAML::Key << "Type" << YAML::Value << materialType;
        out << YAML::Key << "AutoImported" << YAML::Value << autoImported;
        out << YAML::EndMap;
        out << YAML::EndMap;
        fout << out.c_str();
        fout.close();
    }

    void ShaderUtilities::CreateMaterialAssetFile(Handle<Asset> handle, const MaterialDataDescriptor&& desc)
    {
        const auto& path = HBL2::Project::GetAssetFileSystemPath(AssetManager::Instance->GetAssetMetadata(handle)->FilePath);

        if (!std::filesystem::exists(path.parent_path()))
        {
            try
            {
                std::filesystem::create_directories(path.parent_path());
            }
            catch (std::exception& e)
            {
                HBL2_ERROR("Project directory creation failed: {0}", e.what());
            }
        }

        std::ofstream fout(path, std::ios::out | std::ios::trunc);

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Material" << YAML::Value;
        out << YAML::BeginMap;

        if (desc.ShaderAssetHandle.IsValid())
        {
            Asset* asset = AssetManager::Instance->GetAssetMetadata(desc.ShaderAssetHandle);
            out << YAML::Key << "Shader" << YAML::Value << asset->UUID;
        }
        else
        {
            out << YAML::Key << "Shader" << YAML::Value << (UUID)0;
        }

        for (auto& descriptorSet : desc.ReflectionData->descriptorSets)
        {
            if (descriptorSet.set != 2)
            {
                continue;
            }

            uint32_t bufferIndex = 0;
            uint32_t textureIndex = 0;

            for (const auto& b : descriptorSet.bindings)
            {
                if (b.type == ResourceType::UniformBuffer)
                {
                    const std::vector<uint8_t>& uniformBufferBytes = desc.Buffers[bufferIndex++];

                    out << YAML::Key << b.name.c_str();
                    out << YAML::BeginMap;

                    for (const auto& m : b.members)
                    {
                        const uint8_t* memberPtr = uniformBufferBytes.data() + m.offset;

                        switch (m.typeInfo.base)
                        {
                            case MemberBaseType::Float:
                            {
                                const float* f = reinterpret_cast<const float*>(memberPtr);

                                if (m.typeInfo.cols == 1)
                                {
                                    out << YAML::Key << m.name.c_str() << YAML::Value << *f;
                                }
                                else if (m.typeInfo.cols == 2)
                                {
                                    out << YAML::Key << m.name.c_str() << YAML::Value << glm::vec2(f[0], f[1]);
                                }
                                else if (m.typeInfo.cols == 3)
                                {
                                    out << YAML::Key << m.name.c_str() << YAML::Value << glm::vec3(f[0], f[1], f[2]);
                                }
                                else if (m.typeInfo.cols == 4)
                                {
                                    out << YAML::Key << m.name.c_str() << YAML::Value << glm::vec4(f[0], f[1], f[2], f[3]);
                                }
                                break;
                            }
                        }
                    }

                    out << YAML::EndMap;
                }
                else if (b.type == ResourceType::SampledTexture)
                {
                    Handle<Asset> textureAssetHandle = Handle<Asset>::UnPack(desc.TextureAssets[textureIndex++]);

                    if (textureAssetHandle.IsValid())
                    {
                        Asset* asset = AssetManager::Instance->GetAssetMetadata(textureAssetHandle);
                        out << YAML::Key << b.name.c_str() << YAML::Value << asset->UUID;
                    }
                    else
                    {
                        out << YAML::Key << b.name.c_str() << YAML::Value << (UUID)0;
                    }
                }
            }
        }

        out << YAML::Key << "RasterState";
        out << YAML::BeginMap;
        out << YAML::Key << "Topology" << YAML::Value << (int)desc.VariantHash.topology;
        out << YAML::Key << "PolygonMode" << YAML::Value << (int)desc.VariantHash.polygonMode;
        out << YAML::Key << "CullMode" << YAML::Value << (int)desc.VariantHash.cullMode;
        out << YAML::Key << "FrontFace" << YAML::Value << (int)desc.VariantHash.frontFace;
        out << YAML::EndMap;

        out << YAML::Key << "BlendState";
        out << YAML::BeginMap;
        out << YAML::Key << "Enabled" << YAML::Value << (bool)desc.VariantHash.blendEnabled;
        out << YAML::Key << "ColorOutputEnabled" << YAML::Value << (bool)desc.VariantHash.colorOutput;
        out << YAML::EndMap;

        out << YAML::Key << "DepthState";
        out << YAML::BeginMap;
        out << YAML::Key << "Enabled" << YAML::Value << (bool)desc.VariantHash.depthEnabled;
        out << YAML::Key << "WriteEnabled" << YAML::Value << (bool)desc.VariantHash.depthWrite;
        out << YAML::Key << "StencilEnabled" << YAML::Value << (bool)desc.VariantHash.stencilEnabled;
        out << YAML::Key << "DepthTest" << YAML::Value << (int)desc.VariantHash.depthCompare;
        out << YAML::EndMap;

        out << YAML::Key << "ShaderConstantsState";
        out << YAML::BeginMap;
        out << YAML::Key << "ShaderConstantBool0" << YAML::Value << (bool)desc.VariantHash.shaderConstantBool0;
        out << YAML::Key << "ShaderConstantBool1" << YAML::Value << (bool)desc.VariantHash.shaderConstantBool1;
        out << YAML::Key << "ShaderConstantBool2" << YAML::Value << (bool)desc.VariantHash.shaderConstantBool2;
        out << YAML::Key << "ShaderConstantBool3" << YAML::Value << (bool)desc.VariantHash.shaderConstantBool3;
        out << YAML::Key << "ShaderConstantBool4" << YAML::Value << (bool)desc.VariantHash.shaderConstantBool4;
        out << YAML::Key << "ShaderConstantBool5" << YAML::Value << (bool)desc.VariantHash.shaderConstantBool5;
        out << YAML::Key << "ShaderConstantBool6" << YAML::Value << (bool)desc.VariantHash.shaderConstantBool6;
        out << YAML::Key << "ShaderConstantBool7" << YAML::Value << (bool)desc.VariantHash.shaderConstantBool7;
        out << YAML::EndMap;

        out << YAML::EndMap;
        out << YAML::EndMap;

        fout << out.c_str();
        fout.close();
    }

    const char* ShaderUtilities::GetCacheDirectory(GraphicsAPI target)
    {
        switch (target)
        {
        case GraphicsAPI::OPENGL:
            return "assets/cache/shader/opengl";
        case GraphicsAPI::VULKAN:
            return "assets/cache/shader/vulkan";
        default:
            HBL2_CORE_ASSERT(false, "Stage not supported");
            return "";
        }
    }

    void ShaderUtilities::CreateCacheDirectoryIfNeeded(GraphicsAPI target)
    {
        std::string cacheDirectory = GetCacheDirectory(target);

        if (!std::filesystem::exists(cacheDirectory))
        {
            std::filesystem::create_directories(cacheDirectory);
        }
    }

    bool ShaderUtilities::IsVertexStage(int64_t entryPointIndex, int32_t entryPointCount)
    {
        return entryPointCount == 2 && entryPointIndex == 0;
    }

    bool ShaderUtilities::IsFragmentStage(int64_t entryPointIndex, int32_t entryPointCount)
    {
        return entryPointCount == 2 && entryPointIndex == 1;
    }

    bool ShaderUtilities::IsComputeStage(int64_t entryPointIndex, int32_t entryPointCount)
    {
        return entryPointCount == 1 && entryPointIndex == 0;
    }
}
