#include "UnityBuild.h"

namespace HBL2
{
	UnityBuild* UnityBuild::s_Instance = nullptr;

	UnityBuild& UnityBuild::Get()
	{
		HBL2_CORE_ASSERT(s_Instance != nullptr, "UnityBuild::s_Instance is null! Call UnityBuild::Initialize before use.");
		return *s_Instance;
	}

	void UnityBuild::Initialize()
	{
		HBL2_CORE_ASSERT(s_Instance == nullptr, "UnityBuild::s_Instance is not null! UnityBuild::Initialize has been called twice.");
		s_Instance = new UnityBuild;
	}

	void UnityBuild::Shutdown()
	{
		HBL2_CORE_ASSERT(s_Instance != nullptr, "UnityBuild::s_Instance is null!");
		delete s_Instance;
		s_Instance = nullptr;
	}

	bool UnityBuild::Build()
	{
		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);
		return Build(activeScene);
	}

	bool UnityBuild::Build(Scene* ctx)
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

		// Open and inject final code.
		std::ofstream stream(Project::GetProjectDirectory() / "ProjectFiles" / "UnityBuildSource.cpp", std::ios::out);

		if (!stream.is_open())
		{
			HBL2_CORE_ERROR("UnityBuildSource file not found: {0}", Project::GetProjectDirectory() / "UnityBuildSource.cpp");
			return false;
		}

		stream << m_UnityBuildSourceFinal;

		stream.close();

		// Build
		const auto& projectFilesPath = HBL2::Project::GetAssetDirectory().parent_path() / "ProjectFiles";

		// Build the solution					
#ifdef DEBUG
		const std::string& command = R"(""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe" )" + std::filesystem::path(projectFilesPath / "UnityBuild.sln").string() + R"( /t:)" + "UnityBuild" + R"( /p:Configuration=Debug")";
#else
		const std::string& command = R"(""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe" )" + std::filesystem::path(projectFilesPath / "UnityBuild.sln").string() + R"( /t:)" + "UnityBuild" + R"( /p:Configuration=Release")";
#endif // DEBUG

		system(command.c_str());

		NativeScriptUtilities::Get().LoadUnityBuild(ctx);

		return true;
	}

	void UnityBuild::Combine()
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
		std::string projectIncludes;

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

					projectIncludes += std::format("  <ClInclude Include=\"..\\Assets\\{}\" />\n", script->Path.string());
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

		const auto& projectFilesPath = HBL2::Project::GetAssetDirectory().parent_path() / "ProjectFiles";

		// Create solution file for new system
		std::ofstream solutionFile(projectFilesPath / "UnityBuild.sln");

		if (!solutionFile.is_open())
		{
			return;
		}

		solutionFile << NativeScriptUtilities::Get().GetDefaultSolutionText();
		solutionFile.close();

		// Create vcxproj file for new system
		std::ofstream projectFile(projectFilesPath / "UnityBuild.vcxproj");

		if (!projectFile.is_open())
		{
			return;
		}

		projectFile << NativeScriptUtilities::Get().GetDefaultProjectText(projectIncludes);
		projectFile.close();
	}
	
	void UnityBuild::Recompile()
	{
		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		std::vector<std::string> userSystemNames;

		// Store registered user system names.
		for (ISystem* userSystem : activeScene->GetRuntimeSystems())
		{
			if (userSystem->GetType() == SystemType::User)
			{
				userSystemNames.push_back(userSystem->Name);
			}
		}

		std::vector<std::string> userComponentNames;
		std::unordered_map<std::string, std::unordered_map<entt::entity, std::vector<std::byte>>> data;

		// Store all registered meta types.
		for (auto meta_type : entt::resolve(activeScene->GetMetaContext()))
		{
			std::string componentName = meta_type.second.info().name().data();
			componentName = NativeScriptUtilities::Get().CleanComponentNameO3(componentName);
			userComponentNames.push_back(componentName);

			NativeScriptUtilities::Get().SerializeComponents(componentName, activeScene, data);
		}

		// Unload unity build dll.
		NativeScriptUtilities::Get().UnloadUnityBuild(activeScene);

		// Combine all .cpp files in assets in unity build source file.
		Combine();

		// Build unity build source dll.
		Build();

		// Re-register systems.
		for (const auto& userSystemName : userSystemNames)
		{
			NativeScriptUtilities::Get().RegisterSystem(userSystemName, activeScene);
		}

		// Re-register the components.
		for (const auto& userComponentName : userComponentNames)
		{
			NativeScriptUtilities::Get().RegisterComponent(userComponentName, activeScene);
			NativeScriptUtilities::Get().DeserializeComponents(userComponentName, activeScene, data);
		}
	}
	
	bool UnityBuild::Exists()
	{
		const auto& path = NativeScriptUtilities::Get().GetUnityBuildPath();
		return std::filesystem::exists(path);
	}
}

