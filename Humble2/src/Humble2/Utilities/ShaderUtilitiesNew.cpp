#include "ShaderUtilitiesNew.h"

#include "Collections/StaticDArray.h"

#include "slang-com-ptr.h"
#include "slang.h"

namespace HBL2
{
    ShaderUtilitiesNew* ShaderUtilitiesNew::s_Instance = nullptr;

    ShaderUtilitiesNew& ShaderUtilitiesNew::Get()
    {
        HBL2_CORE_ASSERT(s_Instance != nullptr, "ShaderUtilities::s_Instance is null! Call ShaderUtilities::Initialize before use.");
        return *s_Instance;
    }

    void ShaderUtilitiesNew::Initialize()
    {
        HBL2_CORE_ASSERT(s_Instance == nullptr, "ShaderUtilities::s_Instance is not null! ShaderUtilities::Initialize has been called twice.");
        s_Instance = new ShaderUtilitiesNew;
    }

    void ShaderUtilitiesNew::Shutdown()
    {
        HBL2_CORE_ASSERT(s_Instance != nullptr, "ShaderUtilities::s_Instance is null!");

        delete s_Instance;
        s_Instance = nullptr;
    }

    static Slang::ComPtr<slang::IGlobalSession>* g_SLangGlobalSessions;

    ShaderUtilitiesNew::ShaderUtilitiesNew()
    {
        m_Reservation = Allocator::Arena.Reserve("ShaderUtilitiesNewPool", 16_MB);
        m_Arena.Initialize(&Allocator::Arena, 16_MB, m_Reservation);

        uint32_t globalSessionNum = std::thread::hardware_concurrency();

        g_SLangGlobalSessions = (Slang::ComPtr<slang::IGlobalSession>*)m_Arena.Alloc(globalSessionNum * sizeof(Slang::ComPtr<slang::IGlobalSession>));
        g_SLangGlobalSessions = m_Arena.ConstructArray<Slang::ComPtr<slang::IGlobalSession>>(g_SLangGlobalSessions, globalSessionNum);

        {
            HBL2_PROFILE("SLang Global Sessions Creation");

            for (uint32_t i = 0; i < globalSessionNum; i++)
            {
                slang::createGlobalSession(g_SLangGlobalSessions[i].writeRef());
            }
        }
    }    

    std::vector<uint32_t> ShaderUtilitiesNew::Compile(const std::string& shaderFilePath, ShaderReflectionData* outReflectionData, bool forceRecompile)
    {
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
            targetDesc.profile = g_SLangGlobalSessions[workerIndex]->findProfile("spirv_1_5");
            break;
        }

        // Session description.
        slang::SessionDesc sessionDesc = {};
        sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
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

    const char* ShaderUtilitiesNew::GetCacheDirectory(GraphicsAPI target)
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

    void ShaderUtilitiesNew::CreateCacheDirectoryIfNeeded(GraphicsAPI target)
    {
        std::string cacheDirectory = GetCacheDirectory(target);

        if (!std::filesystem::exists(cacheDirectory))
        {
            std::filesystem::create_directories(cacheDirectory);
        }
    }

    std::string ShaderUtilitiesNew::ReadFile(const std::string& filepath)
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
}
