#include "UnityBuilder.h"

namespace HBL2
{
	UnityBuilder* UnityBuilder::s_Instance = nullptr;

	UnityBuilder& UnityBuilder::Get()
	{
		HBL2_CORE_ASSERT(s_Instance != nullptr, "UnityBuilder::s_Instance is null! Call UnityBuilder::Initialize before use.");
		return *s_Instance;
	}

	void UnityBuilder::Initialize()
	{
		HBL2_CORE_ASSERT(s_Instance == nullptr, "UnityBuilder::s_Instance is not null! UnityBuilder::Initialize has been called twice.");
		s_Instance = new UnityBuilder;
	}

	void UnityBuilder::Shutdown()
	{
		HBL2_CORE_ASSERT(s_Instance != nullptr, "UnityBuilder::s_Instance is null!");
		delete s_Instance;
		s_Instance = nullptr;
	}

	bool UnityBuilder::Build()
	{
		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);
		const std::string& projectName = Project::GetActive()->GetName();

		// Create directory.
		try
		{
			std::filesystem::create_directories(Project::GetProjectDirectory() / "ProjectFiles" / activeScene->GetName());
		}
		catch (std::exception& e)
		{
			HBL2_ERROR("Project directory creation failed: {0}", e.what());
		}

		// Open and inject final code.
		std::ofstream stream(Project::GetProjectDirectory() / "ProjectFiles" / activeScene->GetName() / "UnityBuildSource.cpp", std::ios::out);

		if (!stream.is_open())
		{
			HBL2_CORE_ERROR("UnityBuildSource file not found: {0}", Project::GetProjectDirectory() / "ProjectFiles" / activeScene->GetName() / "UnityBuildSource.cpp");
			return false;
		}

		stream << m_UnityBuildSourceFinal;

		stream.close();

		// Build
		const auto& projectFilesPath = HBL2::Project::GetAssetDirectory().parent_path() / "ProjectFiles";

#ifdef DEBUG
		const auto& dllPath = std::filesystem::path("assets") / "dlls" / "Debug-x86_64" / projectName / activeScene->GetName() / "UnityBuild.dll";
#else
		const auto& dllPath = std::filesystem::path("assets") / "dlls" / "Release-x86_64" / projectName / activeScene->GetName() / "UnityBuild.dll";
#endif

		if (activeScene != nullptr)
		{
			// Remove old dll
			try
			{
				if (std::filesystem::remove(dllPath))
				{
					std::cout << "file " << dllPath << " deleted.\n";
				}
				else
				{
					std::cout << "file " << dllPath << " not found.\n";
				}
			}
			catch (const std::filesystem::filesystem_error& err)
			{
				std::cout << "filesystem error: " << err.what() << '\n';
			}
		}

		// Build the solution					
#ifdef DEBUG
		const std::string& command = R"(""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe" )" + std::filesystem::path(projectFilesPath / activeScene->GetName() / "UnityBuild.sln").string() + R"( /t:)" + "UnityBuild" + R"( /p:Configuration=Debug")";
#else
		const std::string& command = R"(""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe" )" + std::filesystem::path(projectFilesPath / activeScene->GetName() / "UnityBuild.sln").string() + R"( /t:)" + "UnityBuild" + R"( /p:Configuration=Release")";
#endif // DEBUG
		system(command.c_str());

		NativeScriptUtilities::Get().LoadUnityBuild(activeScene);

		return true;
	}

	void UnityBuilder::Combine()
	{
		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		// Create directory.
		try
		{
			std::filesystem::create_directories(Project::GetProjectDirectory() / "ProjectFiles" / activeScene->GetName());
		}
		catch (std::exception& e)
		{
			HBL2_ERROR("Project directory creation failed: {0}", e.what());
		}

		m_UnityBuildSourceFinal = m_UnityBuildSource;

		// Collect includes and systems registration
		std::string includes;
		std::string projectIncludes;
		std::string systemsRegistration;

		for (const auto assetHandle : AssetManager::Instance->GetRegisteredAssets())
		{
			Asset* asset = AssetManager::Instance->GetAssetMetadata(assetHandle);

			if (asset->Type == AssetType::Script)
			{
				Handle<Script> scriptHandle = Handle<Script>::UnPack(asset->Indentifier);

				if (scriptHandle.IsValid())
				{
					Script* script = ResourceManager::Instance->GetScript(scriptHandle);

					if (script->Type == ScriptType::UNITY_BUILD_SCRIPT)
					{
						continue;
					}

					includes += std::format("#include \"{}\"\n", script->Path.string());
					projectIncludes += std::format("  <ClInclude Include=\"..\Assets\{}\" />\n", script->Path.string());

					const std::string& scriptName = std::string(script->Name);

					switch (script->Type)
					{
					case ScriptType::SYSTEM:
						systemsRegistration += std::format("HBL2::ISystem* new{} = new {};\n\tnew{}->Name = \"{}\";\n\tctx->RegisterSystem(new{}, HBL2::SystemType::User);\n", scriptName, std::string(script->Name), scriptName, scriptName, scriptName);
						break;
					}
				}
			}
		}

		// Inject includes in the code.
		{
			const std::string& placeholder = "{Includes}";

			size_t pos = m_UnityBuildSourceFinal.find(placeholder);

			while (pos != std::string::npos)
			{
				((std::string&)m_UnityBuildSourceFinal).replace(pos, placeholder.length(), includes);
				pos = m_UnityBuildSourceFinal.find(placeholder, pos + includes.length());
			}
		}

		// Inject systems to register in the code.
		{
			const std::string& placeholder = "{SystemsRegistration}";

			size_t pos = m_UnityBuildSourceFinal.find(placeholder);

			while (pos != std::string::npos)
			{
				((std::string&)m_UnityBuildSourceFinal).replace(pos, placeholder.length(), systemsRegistration);
				pos = m_UnityBuildSourceFinal.find(placeholder, pos + systemsRegistration.length());
			}
		}

		const auto& projectFilesPath = HBL2::Project::GetAssetDirectory().parent_path() / "ProjectFiles";

		// Create solution file for new system
		std::ofstream solutionFile(projectFilesPath / activeScene->GetName() / "UnityBuild.sln");

		if (!solutionFile.is_open())
		{
			return;
		}

		solutionFile << NativeScriptUtilities::Get().GetDefaultSolutionText("");
		solutionFile.close();

		// Create vcxproj file for new system
		std::ofstream projectFile(projectFilesPath / activeScene->GetName() / "UnityBuild.vcxproj");

		if (!projectFile.is_open())
		{
			return;
		}

		projectFile << NativeScriptUtilities::Get().GetDefaultProjectText(projectIncludes);
		projectFile.close();
	}

	void UnityBuilder::LoadUnityBuildScript()
	{
		auto script = ResourceManager::Instance->CreateScript({
			.debugName = "UnityBuildSource",
			.type = ScriptType::UNITY_BUILD_SCRIPT,
			.path = Project::GetProjectDirectory() / "ProjectFiles",
		});
	}
}

