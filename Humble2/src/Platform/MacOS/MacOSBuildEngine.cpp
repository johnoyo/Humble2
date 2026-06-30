#include "MacOSBuildEngine.h"

#include "Project/Project.h"
#include "Utilities/FileDialogs.h"

#include <cstdio>

namespace HBL2
{
    static std::string ReadFile(const std::filesystem::path& filepath)
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
                HBL2_CORE_ERROR("Could not read from file '{0}'", filepath.string());
            }
        }
        else
        {
            HBL2_CORE_ERROR("Could not open file '{0}'", filepath.string());
        }

        return result;
    }

    static int ExecuteCommand(const char* command)
    {
        FILE* pipe = popen(command, "r");
        if (!pipe)
        {
            return -1;
        }

        char buf[256];
        std::string output;
        while (fgets(buf, sizeof(buf), pipe))
        {
            output += buf;
        }
        
        HBL2_CORE_INFO(output);

        return pclose(pipe);
    }

    bool MacOSBuildEngine::Build()
    {
        // Combine scripts into a single cpp source file.
        Combine();
        
        // Create directory.
        const auto& projectFilesPath = Project::GetProjectDirectory() / "ProjectFiles";
        
        try
        {
            std::filesystem::create_directories(projectFilesPath);
        }
        catch (std::exception& e)
        {
            HBL2_ERROR("Project directory creation failed: {0}", e.what());
        }

        // Open and inject final code.
        std::ofstream stream(projectFilesPath / "UnityBuildSource.cpp", std::ios::out);

        if (!stream.is_open())
        {
            HBL2_CORE_ERROR("UnityBuildSource file not found: {0}", projectFilesPath / "UnityBuildSource.cpp");
            return false;
        }

        stream << m_UnityBuildSourceFinal;
        stream.close();
        
        // Patch project name in premake file.
        const std::string& projectName = Project::GetActive()->GetName();
        const auto& premakeSource = ReadFile(projectFilesPath / "premake5.lua");
        const std::string& placeholder = "{ProjectName}";

        size_t pos = premakeSource.find(placeholder);

        while (pos != std::string::npos)
        {
            ((std::string&)premakeSource).replace(pos, placeholder.length(), projectName);
            pos = premakeSource.find(placeholder, pos + projectName.length());
        }
        
        // Write back the injected premake source into file.
        std::ofstream streamPremake(projectFilesPath / "premake5.lua", std::ios::out);

        if (!streamPremake.is_open())
        {
            HBL2_CORE_ERROR("premake5.lua file not found: {0}", projectFilesPath / "premake5.lua");
            return false;
        }

        streamPremake << premakeSource;
        streamPremake.close();
        
        // Create xcode project through premake.
        const auto& premakeCmd = "cd " + projectFilesPath.string() + " && ./../../../Dependencies/Premake5/MacOS/premake5 xcode4";
        
        int cmdExecutionRes = ExecuteCommand(premakeCmd.c_str());
        
        if (cmdExecutionRes != 0)
        {
            HBL2_CORE_ERROR("XCode project files generation through premake failed.");
            return false;
        }
        
        // Build xcode project.
        std::string xcodebuildCmd;
        
        switch (m_CurrentConfiguration)
        {
        case BuildEngine::Configuration::Debug:
            xcodebuildCmd = "xcodebuild -workspace " + projectFilesPath.string() + "/UnityBuild.xcworkspace -scheme UnityBuild -configuration Debug build";
            cmdExecutionRes = ExecuteCommand(xcodebuildCmd.c_str());
            break;
        case BuildEngine::Configuration::Release:
        case BuildEngine::Configuration::Distribution:
            xcodebuildCmd = "xcodebuild -workspace " + projectFilesPath.string() + "/UnityBuild.xcworkspace -scheme UnityBuild -configuration Release build";
            cmdExecutionRes = ExecuteCommand(xcodebuildCmd.c_str());
            break;
        }
        
        if (cmdExecutionRes != 0)
        {
            HBL2_CORE_ERROR("XCode project build failed.");
            return false;
        }
        
        // Load dynamic library.
        LoadBuild(m_CurrentConfiguration);
        
        return true;
    }

    bool MacOSBuildEngine::RunRuntime(Configuration configuration)
    {
        int commandExecutionResult = 0;
        
        switch (configuration)
        {
        case BuildEngine::Configuration::Debug:
            commandExecutionResult = ExecuteCommand("cd ../../Debug-x86_64/HumbleApp/HumbleApp.app/Contents && ./MacOS/HumbleApp");
            break;
        case BuildEngine::Configuration::Release:
            commandExecutionResult = ExecuteCommand("cd ../../Release-x86_64/HumbleApp/HumbleApp.app/Contents && ./MacOS/HumbleApp");
            break;
        case BuildEngine::Configuration::Distribution:
            commandExecutionResult = ExecuteCommand("cd ../../Dist-x86_64/HumbleApp/HumbleApp.app/Contents && ./MacOS/HumbleApp");
            break;
        }

        return (commandExecutionResult == 0);
    }

    bool MacOSBuildEngine::BuildRuntime(Configuration configuration)
    {
        const std::string& projectName = Project::GetActive()->GetName();
        const auto& workingDir = Project::GetProjectDirectory().parent_path();
        
        int commandExecutionResult = 0;
        
        std::string config;
        
        switch (configuration)
        {
            case Configuration::Debug:
                config = "Debug";
                commandExecutionResult = ExecuteCommand("xcodebuild -workspace ../../../HumbleGameEngine2.xcworkspace -scheme HumbleApp -configuration Debug build");
                break;
            case Configuration::Release:
                config = "Release";
                commandExecutionResult = ExecuteCommand("xcodebuild -workspace ../../../HumbleGameEngine2.xcworkspace -scheme HumbleApp -configuration Release build");
                break;
            case Configuration::Distribution:
                config = "Dist";
                commandExecutionResult = ExecuteCommand("xcodebuild -workspace ../../../HumbleGameEngine2.xcworkspace -scheme HumbleApp -configuration Dist build");
                break;
        }
        
        if (commandExecutionResult == 0)
        {
            // Copy project folder to build folder.
            FileUtils::CopyFolder(workingDir / projectName, "../../" + config + "-x86_64/HumbleApp/HumbleApp.app/Contents/Resources/" + projectName);

            // Copy assets folder to build folder.
            FileUtils::CopyFolder(workingDir / "assets", "../../" + config + "-x86_64/HumbleApp/HumbleApp.app/Contents/Resources/assets");
            
            return true;
        }
        
        return false;
    }

    const std::filesystem::path MacOSBuildEngine::GetUnityBuildPath(Configuration config) const
    {
        const std::string& projectName = Project::GetActive()->GetName();
        const auto& rootPath = Project::GetProjectDirectory().parent_path();
        
        switch (config)
        {
        case Configuration::Debug:
            return rootPath / "assets" / "dlls" / "Debug-x86_64" / projectName / "libUnityBuild.dylib";
        case Configuration::Release:
        case Configuration::Distribution:
            return rootPath / "assets" / "dlls" / "Release-x86_64" / projectName / "libUnityBuild.dylib";
        }

        return std::filesystem::path("");
    }

    void MacOSBuildEngine::Combine()
    {
        // Create directory.
        try
        {
            std::filesystem::create_directories(Project::GetProjectDirectory() / "ProjectFiles");
        }
        catch (std::exception& e)
        {
            HBL2_ERROR("Project directory creation failed: {0}", e.what());
        }

        m_UnityBuildSourceFinal = m_UnityBuildSource;

        // Collect includes and systems registration
        std::string componentIncludes;
        std::string helperScriptsIncludes;
        std::string systemIncludes;

        for (const auto assetHandle : AssetManager::Instance->GetRegisteredAssets())
        {
            if (!AssetManager::Instance->IsAssetValid(assetHandle))
            {
                continue;
            }

            Asset* asset = AssetManager::Instance->GetAssetMetadata(assetHandle);

            if (asset->Type == AssetType::Script)
            {
                Handle<Script> scriptHandle = AssetManager::Instance->GetAsset<Script>(asset->UUID);

                if (scriptHandle.IsValid())
                {
                    Script* script = ResourceManager::Instance->GetScript(scriptHandle);

                    if (script->Type == ScriptType::COMPONENT)
                    {
                        componentIncludes += std::format("#include \"{}\"\n", script->Path.string());
                    }
                    else if (script->Type == ScriptType::SYSTEM)
                    {
                        systemIncludes += std::format("#include \"{}\"\n", script->Path.string());
                    }
                    else if (script->Type == ScriptType::HELPER_SCRIPT)
                    {
                        helperScriptsIncludes += std::format("#include \"{}\"\n", script->Path.string());
                    }
                }
            }
        }

        // Inject includes in the code.
        {
            const std::string& placeholder = "{ComponentIncludes}";

            size_t pos = m_UnityBuildSourceFinal.find(placeholder);

            while (pos != std::string::npos)
            {
                ((std::string&)m_UnityBuildSourceFinal).replace(pos, placeholder.length(), componentIncludes);
                pos = m_UnityBuildSourceFinal.find(placeholder, pos + componentIncludes.length());
            }
        }

        {
            const std::string& placeholder = "{HelperScriptIncludes}";

            size_t pos = m_UnityBuildSourceFinal.find(placeholder);

            while (pos != std::string::npos)
            {
                ((std::string&)m_UnityBuildSourceFinal).replace(pos, placeholder.length(), helperScriptsIncludes);
                pos = m_UnityBuildSourceFinal.find(placeholder, pos + helperScriptsIncludes.length());
            }
        }

        {
            const std::string& placeholder = "{SystemIncludes}";

            size_t pos = m_UnityBuildSourceFinal.find(placeholder);

            while (pos != std::string::npos)
            {
                ((std::string&)m_UnityBuildSourceFinal).replace(pos, placeholder.length(), systemIncludes);
                pos = m_UnityBuildSourceFinal.find(placeholder, pos + systemIncludes.length());
            }
        }
    }
}
