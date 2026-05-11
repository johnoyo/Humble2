#include "ShaderUtilities.h"

#include "YamlUtilities.h"
#include "Collections/StaticDArray.h"

#include "Project\Project.h"

#include "slang-com-ptr.h"
#include "slang.h"

namespace HBL2
{
    ShaderUtilities* ShaderUtilities::s_Instance = nullptr;

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
        baseVariant["BlendState"]["Enabled"] = (bool)variant.blendEnabled;
        baseVariant["BlendState"]["ColorOutputEnabled"] = (bool)variant.colorOutput;

        baseVariant["DepthState"]["Enabled"] = (bool)variant.depthEnabled;
        baseVariant["DepthState"]["WriteEnabled"] = (bool)variant.depthWrite;
        baseVariant["DepthState"]["StencilEnabled"] = (bool)variant.stencilEnabled;
        baseVariant["DepthState"]["DepthTest"] = (int)variant.depthCompare;

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

    static Slang::ComPtr<slang::IGlobalSession>* g_SLangGlobalSessions;

    ShaderUtilities::ShaderUtilities()
    {
        m_Reservation = Allocator::Arena.Reserve("ShaderUtilitiesPool", 16_MB);
        m_Arena.Initialize(&Allocator::Arena, 16_MB, m_Reservation);

        m_ShaderAssets = MakeDArray<Handle<Asset>>(m_Arena, 1024);
        m_Shaders = MakeHMap<BuiltInShader, Handle<Shader>>(m_Arena, 1024);
        m_ShaderLayouts = MakeHMap<BuiltInShader, Handle<BindGroupLayout>>(m_Arena, 64);

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

    std::vector<uint32_t> ShaderUtilities::CompileAlt(const std::string& shaderFilePath, ShaderReflectionData* outReflectionData, bool forceRecompile)
    {
        HBL2_FUNC_PROFILE();

        GraphicsAPI target = Renderer::Instance->GetAPI();
        CreateCacheDirectoryIfNeeded(target);

        std::filesystem::path shaderPath = shaderFilePath;
        std::filesystem::path cacheDirectory = GetCacheDirectory(target);
        std::filesystem::path cachedPath = cacheDirectory / (shaderPath.filename().string() + ".cached.spv");

        // Check cache for hit.
        {
            std::ifstream in(cachedPath, std::ios::in | std::ios::binary);
            if (in.is_open() && !forceRecompile)
            {
                in.seekg(0, std::ios::end);
                auto size = in.tellg();
                in.seekg(0, std::ios::beg);

                std::vector<uint32_t> cached(size / sizeof(uint32_t));
                in.read(reinterpret_cast<char*>(cached.data()), size);
                return cached;
            }
        }

        std::string shaderSource = ReadFile(shaderFilePath);

        uint32_t workerIndex = JobSystem::Get().GetWorkerIndex();

        // Target description.
        slang::TargetDesc targetDesc = {};
        targetDesc.format = SLANG_SPIRV;
        targetDesc.flags = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY | SLANG_TARGET_FLAG_GENERATE_WHOLE_PROGRAM;

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

        std::array<slang::CompilerOptionEntry, 2> options =
        {
            slang::CompilerOptionEntry
            {
                .name = slang::CompilerOptionName::EmitSpirvDirectly,
                .value = { .kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1 },
            },
            slang::CompilerOptionEntry
            {
                .name = slang::CompilerOptionName::GenerateWholeProgram,
                .value = {.kind = slang::CompilerOptionValueKind::Int, .intValue0 = 1 },
            },
            /*slang::CompilerOptionEntry
            {
                .name = slang::CompilerOptionName::Optimization,
                .value = {.kind = slang::CompilerOptionValueKind::Int, .intValue0 = SlangOptimizationLevel::SLANG_OPTIMIZATION_LEVEL_NONE }
            }*/
        };

        sessionDesc.compilerOptionEntries = options.data();
        sessionDesc.compilerOptionEntryCount = options.size();

        sessionDesc.targets = &targetDesc;
        sessionDesc.targetCount = 1;

        Slang::ComPtr<slang::ISession> session;
        {
            if (SLANG_FAILED(g_SLangGlobalSessions[workerIndex]->createSession(sessionDesc, session.writeRef())))
            {
                HBL2_CORE_ERROR("Slang: Failed to create session");
                return std::vector<uint32_t>();
            }
        }

        // SLang module creation.
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
            return std::vector<uint32_t>();
        }

        // Entry points.
        StaticDArray<Slang::ComPtr<slang::IEntryPoint>, 6> entryPoints;
        SlangInt32 entryPointCount = slangModule->getDefinedEntryPointCount();
        if (entryPointCount == 0)
        {
            HBL2_CORE_ERROR("Slang: No entry points found in shader source");
            return std::vector<uint32_t>();
        }

        for (SlangInt32 i = 0; i < entryPointCount; ++i)
        {
            Slang::ComPtr<slang::IEntryPoint> ep;
            slangModule->getDefinedEntryPoint(i, ep.writeRef());
            entryPoints.push_back(std::move(ep));
        }

        // Components.
        StaticDArray<slang::IComponentType*, 6> components;
        components.push_back(slangModule);
        for (auto& ep : entryPoints)
        {
            components.push_back(ep.get());
        }

        // Link program.
        Slang::ComPtr<slang::IComponentType> linkedProgram;
        if (SLANG_FAILED(session->createCompositeComponentType(components.data(), (SlangInt)components.size(), linkedProgram.writeRef(), diagnostics.writeRef())))
        {
            if (diagnostics)
            {
                HBL2_CORE_ERROR("Slang link error: {}", (const char*)diagnostics->getBufferPointer());
            }
            HBL2_CORE_ERROR("Slang: Failed to link shader program");
            return std::vector<uint32_t>();
        }

        // Get binary code.
        std::vector<uint32_t> binaries;
        for (SlangInt i = 0; i < (SlangInt)entryPoints.size(); ++i)
        {
            Slang::ComPtr<slang::IBlob> code;
            diagnostics = nullptr;

            if (SLANG_FAILED(linkedProgram->getEntryPointCode(i, 0, code.writeRef(), diagnostics.writeRef())))
            {
                if (diagnostics)
                {
                    HBL2_CORE_ERROR("Slang compile error: {}", (const char*)diagnostics->getBufferPointer());
                }

                HBL2_CORE_ERROR("Slang: Failed to get SPIR-V for entry point {}", i);
                return std::vector<uint32_t>();
            }

            if (diagnostics)
            {
                HBL2_CORE_WARN("Slang: {}", (const char*)diagnostics->getBufferPointer());
            }

            const uint32_t* SpirvData = static_cast<const uint32_t*>(code->getBufferPointer());
            size_t SpirvSize = code->getBufferSize() / sizeof(uint32_t);
            binaries.insert(binaries.end(), SpirvData, SpirvData + SpirvSize);
        }

        if (binaries.empty())
        {
            HBL2_CORE_ERROR("Slang: Shader compiled to empty SPIR-V");
            return std::vector<uint32_t>();
        }               

        std::ofstream out(cachedPath, std::ios::out | std::ios::binary);
        if (out.is_open())
        {
            out.write((char*)binaries.data(), binaries.size() * sizeof(uint32_t));
            out.flush();
            out.close();
        }

        // Reflection
        if (outReflectionData != nullptr)
        {
            *outReflectionData = ShaderReflector::Reflect(linkedProgram, shaderFilePath);
        }

        return binaries;
    }

    void ShaderUtilities::LoadBuiltInShaders()
    {
        auto drawBindGroupLayout0 = ResourceManager::Instance->CreateBindGroupLayout({
            .debugName = "built-in-simple-lit-bind-group-layout",
            .textureBindings = {
                /*
                * Here we start the texture binds from zero despite having already buffers bound there.
                * Thats because in a set each type (buffer, image, etc) has unique binding points.
                * This also comes in handy in openGL when binding textures.
                */
                {
                    .slot = 0,
                    .visibility = ShaderStage::FRAGMENT,
                },
                {
                    .slot = 3,
                    .visibility = ShaderStage::FRAGMENT,
                },
            },
            .bufferBindings = {
                {
                    /*
                    * We use binding 2 since despite being in another bind group with no other bindings,
                    * for compatibility with opengl we must have unique bindings for buffers,
                    * so since 0 and 1 are used in the global bind group we must use the slot 2 here.
                    */
                    .slot = 2,
                    .visibility = ShaderStage::VERTEX,
                    .type = BufferBindingType::UNIFORM_DYNAMIC_OFFSET,
                },
            },
        });
        m_ShaderLayouts[BuiltInShader::INVALID] = drawBindGroupLayout0;
        m_ShaderLayouts[BuiltInShader::PRESENT] = {};
        m_ShaderLayouts[BuiltInShader::UNLIT] = drawBindGroupLayout0;
        m_ShaderLayouts[BuiltInShader::BLINN_PHONG] = drawBindGroupLayout0;

        auto drawBindGroupLayout1 = ResourceManager::Instance->CreateBindGroupLayout({
            .debugName = "built-in-lit-bind-group-layout",
            .textureBindings = {
                /*
                * Here we start the texture binds from zero despite having already buffers bound there.
                * Thats because in a set each type (buffer, image, etc) has unique binding points.
                * This also comes in handy in openGL when binding textures.
                */
                {
                    .slot = 0,
                    .visibility = ShaderStage::FRAGMENT,
                },
                {
                    .slot = 1,
                    .visibility = ShaderStage::FRAGMENT,
                },
                {
                    .slot = 2,
                    .visibility = ShaderStage::FRAGMENT,
                },
                {
                    .slot = 3,
                    .visibility = ShaderStage::FRAGMENT,
                },
                {
                    .slot = 4,
                    .visibility = ShaderStage::FRAGMENT,
                },
            },
            .bufferBindings = {
                {
                    /*
                    * We use binding 5 since despite being a different type of resource,
                    * each binding within a descriptor set must be unique, so since we have 0, 1, 2, 3, 4
                    * used by textures, the next empty is 5.
                    *
                    */
                    .slot = 5,
                    .visibility = ShaderStage::VERTEX,
                    .type = BufferBindingType::UNIFORM_DYNAMIC_OFFSET,
                },
            },
        });
        m_ShaderLayouts[BuiltInShader::PBR] = drawBindGroupLayout1;

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

        CreateShaderMetadataFile(pbrShaderAssetHandle, 2);
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
        for (auto& [shaderType, layoutHandle] : m_ShaderLayouts)
        {
            ResourceManager::Instance->DeleteBindGroupLayout(layoutHandle);
        }

        m_ShaderLayouts.clear();

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

        Handle<Shader> shaderHandle = AssetManager::Instance->GetAsset<Shader>(handle);
        ShaderDescriptor::RenderPipeline::PackedVariant variantHash = {};

        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Shader" << YAML::Value;
        out << YAML::BeginMap;
        out << YAML::Key << "UUID" << YAML::Value << asset->UUID;
        out << YAML::Key << "Type" << YAML::Value << shaderType;

        out << YAML::Key << "Variants" << YAML::BeginSeq;
        out << YAML::BeginMap;
        out << YAML::Key << "Variant" << YAML::Value << variantHash.Key();

        out << YAML::Key << "BlendState";
        out << YAML::BeginMap;
        out << YAML::Key << "Enabled" << YAML::Value << true;
        out << YAML::Key << "ColorOutputEnabled" << YAML::Value << true;
        out << YAML::EndMap;

        out << YAML::Key << "DepthState";
        out << YAML::BeginMap;
        out << YAML::Key << "Enabled" << YAML::Value << true;
        out << YAML::Key << "WriteEnabled" << YAML::Value << true;
        out << YAML::Key << "StencilEnabled" << YAML::Value << true;
        out << YAML::Key << "DepthTest" << YAML::Value << (int)Compare::LESS;
        out << YAML::EndMap;

        out << YAML::EndMap;
        out << YAML::EndSeq;

        out << YAML::EndMap;
        out << YAML::EndMap;

        fout << out.c_str();
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

        out << YAML::Key << "AlbedoColor" << YAML::Value << desc.AlbedoColor;
        out << YAML::Key << "Glossiness" << YAML::Value << desc.Glossiness;

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

        if (desc.AlbedoMapAssetHandle.IsValid())
        {
            Asset* asset = AssetManager::Instance->GetAssetMetadata(desc.AlbedoMapAssetHandle);
            out << YAML::Key << "AlbedoMap" << YAML::Value << asset->UUID;
        }
        else
        {
            out << YAML::Key << "AlbedoMap" << YAML::Value << (UUID)0;
        }

        if (desc.NormalMapAssetHandle.IsValid())
        {
            Asset* asset = AssetManager::Instance->GetAssetMetadata(desc.NormalMapAssetHandle);
            out << YAML::Key << "NormalMap" << YAML::Value << asset->UUID;
        }
        else
        {
            out << YAML::Key << "NormalMap" << YAML::Value << (UUID)0;
        }

        if (desc.MetallicMapAssetHandle.IsValid())
        {
            Asset* asset = AssetManager::Instance->GetAssetMetadata(desc.MetallicMapAssetHandle);
            out << YAML::Key << "MetallicMap" << YAML::Value << asset->UUID;
        }
        else
        {
            out << YAML::Key << "MetallicMap" << YAML::Value << (UUID)0;
        }

        if (desc.RoughnessMapAssetHandle.IsValid())
        {
            Asset* asset = AssetManager::Instance->GetAssetMetadata(desc.RoughnessMapAssetHandle);
            out << YAML::Key << "RoughnessMap" << YAML::Value << asset->UUID;
        }
        else
        {
            out << YAML::Key << "RoughnessMap" << YAML::Value << (UUID)0;
        }

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
