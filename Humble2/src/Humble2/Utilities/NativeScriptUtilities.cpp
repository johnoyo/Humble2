#include "NativeScriptUtilities.h"

namespace HBL2
{
	extern "C"
	{
		typedef ISystem* (*CreateSystemFunc)();

		typedef void (*RegisterSystemsFunc)(Scene*);
		
		typedef const char* (*RegisterComponentFunc)(entt::meta_ctx*);

		typedef entt::meta_any (*AddNewComponentFunc)(Scene*, entt::entity);

		typedef entt::meta_any (*GetNewComponentFunc)(Scene*, entt::entity);

		typedef bool (*HasNewComponentFunc)(Scene*, entt::entity);
	}

	NativeScriptUtilities* NativeScriptUtilities::s_Instance = nullptr;

	NativeScriptUtilities& NativeScriptUtilities::Get()
	{
		HBL2_CORE_ASSERT(s_Instance != nullptr, "NativeScriptUtilities::s_Instance is null! Call NativeScriptUtilities::Initialize before use.");
		return *s_Instance;
	}

	void NativeScriptUtilities::Initialize()
	{
		HBL2_CORE_ASSERT(s_Instance == nullptr, "NativeScriptUtilities::s_Instance is not null! NativeScriptUtilities::Initialize has been called twice.");

		s_Instance = new NativeScriptUtilities;
	}

	void NativeScriptUtilities::Shutdown()
	{
		HBL2_CORE_ASSERT(s_Instance != nullptr, "NativeScriptUtilities::s_Instance is null!");

		delete s_Instance;
		s_Instance = nullptr;
	}

	Handle<Asset> NativeScriptUtilities::CreateSystemFile(const std::filesystem::path& currentDir, const std::string& systemName)
	{
		auto relativePath = std::filesystem::relative(currentDir / (systemName + ".h"), HBL2::Project::GetAssetDirectory());

		auto scriptAssetHandle = AssetManager::Instance->CreateAsset({
			.debugName = "script-asset",
			.filePath = relativePath,
			.type = AssetType::Script,
		});

		if (scriptAssetHandle.IsValid())
		{
			std::ofstream fout(HBL2::Project::GetAssetFileSystemPath(relativePath).string() + ".hblscript", 0);
			YAML::Emitter out;
			out << YAML::BeginMap;
			out << YAML::Key << "Script" << YAML::Value;
			out << YAML::BeginMap;
			out << YAML::Key << "UUID" << YAML::Value << AssetManager::Instance->GetAssetMetadata(scriptAssetHandle)->UUID;
			out << YAML::Key << "Type" << YAML::Value << (uint32_t)ScriptType::SYSTEM;
			out << YAML::EndMap;
			out << YAML::EndMap;
			fout << out.c_str();
			fout.close();
		}

		std::ofstream fout(currentDir / (systemName + ".h"), 0);
		fout << GetDefaultSystemCode(systemName);
		fout.close();

		return scriptAssetHandle;
	}

	ISystem* NativeScriptUtilities::GenerateSystem(const std::string& systemName)
	{
		// Create system file
		std::ofstream systemFile(HBL2::Project::GetAssetDirectory() / "Scripts" / (systemName + ".cpp"), 0);
		systemFile << NativeScriptUtilities::Get().GetDefaultSystemCode(systemName);
		systemFile.close();

		// Create folder in ProjectFiles
		const auto& projectFilesPath = HBL2::Project::GetAssetDirectory().parent_path() / "ProjectFiles" / systemName;

		try
		{
			std::filesystem::create_directories(projectFilesPath);
		}
		catch (std::exception& e)
		{
			HBL2_ERROR("Project directory creation failed: {0}", e.what());
		}

		// Create solution file for new system
		std::ofstream solutionFile(projectFilesPath / (systemName + ".sln"));

		if (!solutionFile.is_open())
		{
			return nullptr;
		}

		solutionFile << NativeScriptUtilities::Get().GetDefaultSolutionText(systemName);
		solutionFile.close();

		// Create vcxproj file for new system
		std::ofstream projectFile(projectFilesPath / (systemName + ".vcxproj"));

		if (!projectFile.is_open())
		{
			return nullptr;
		}

		projectFile << NativeScriptUtilities::Get().GetDefaultProjectText(systemName);
		projectFile.close();

		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		if (activeScene != nullptr)
		{
			activeScene->DeregisterSystem(systemName);

			// Remove old dll
			try
			{
				if (std::filesystem::remove(std::filesystem::path("assets") / "dlls" / (systemName + ".dll")))
				{
					std::cout << "file " << std::filesystem::path("assets") / "dlls" / (systemName + ".dll") << " deleted.\n";
				}
				else
				{
					std::cout << "file " << std::filesystem::path("assets") / "dlls" / (systemName + ".dll") << " not found.\n";
				}
			}
			catch (const std::filesystem::filesystem_error& err)
			{
				std::cout << "filesystem error: " << err.what() << '\n';
			}
		}

		// Build the solution					
#ifdef DEBUG
		const std::string& command = R"(""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe" )" + std::filesystem::path(projectFilesPath / (systemName + ".sln")).string() + R"( /t:)" + systemName + R"( /p:Configuration=Debug")";
#else
		const std::string& command = R"(""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe" )" + std::filesystem::path(projectFilesPath / (systemName + ".sln")).string() + R"( /t:)" + systemName + R"( /p:Configuration=Release")";
#endif // DEBUG
		system(command.c_str());

		// Load system dll
		const std::string& dllPath = "assets\\dlls\\" + systemName + "\\" + systemName + ".dll";
		ISystem* newSystem = NativeScriptUtilities::Get().LoadSystem(dllPath, activeScene);

		HBL2_CORE_ASSERT(newSystem != nullptr, "Failed to load system.");

		return newSystem;
	}

	ISystem* NativeScriptUtilities::CompileSystem(const std::string& systemName)
	{
		const std::string& dllPath = (std::filesystem::path("assets") / "dlls" / systemName / (systemName + ".dll")).string();

		const auto& projectFilesPath = HBL2::Project::GetAssetDirectory().parent_path() / "ProjectFiles" / systemName;

		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		if (activeScene != nullptr)
		{
			activeScene->DeregisterSystem(systemName);
		}

		// Create new vcxproj file for system
		std::ofstream projectFile(projectFilesPath / (systemName + ".vcxproj"));

		if (!projectFile.is_open())
		{
			return nullptr;
		}

		bool dllLocked = true;

		while (dllLocked)
		{
			try
			{
				if (std::filesystem::remove(dllPath))
				{
					dllLocked = false;
					std::cout << "file " << dllPath << " deleted.\n";
				}
				else
				{
					dllLocked = false;
					std::cout << "file " << dllPath << " not found.\n";
				}
			}
			catch (const std::filesystem::filesystem_error& err)
			{
				dllLocked = true;
				std::cout << "filesystem error: " << err.what() << '\n';
			}
		}

		projectFile << NativeScriptUtilities::Get().GetDefaultProjectText(systemName);
		projectFile.close();

		// Build the solution
#ifdef DEBUG
		const std::string& command = R"(""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe" )" + std::filesystem::path(projectFilesPath / (systemName + ".sln")).string() + R"( /t:)" + systemName + R"( /p:Configuration=Debug")";
#else
		const std::string& command = R"(""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe" )" + std::filesystem::path(projectFilesPath / (systemName + ".sln")).string() + R"( /t:)" + systemName + R"( /p:Configuration=Release")";
#endif // DEBUG
		system(command.c_str());

		// Load dll
		ISystem* newSystem = NativeScriptUtilities::Get().LoadSystem(dllPath, activeScene);

		HBL2_CORE_ASSERT(newSystem != nullptr, "Failed to load system.");

		return newSystem;
	}

	std::string NativeScriptUtilities::GetDefaultSystemCode(const std::string& systemName)
	{
		const std::string& placeholder = "{SystemName}";

		const std::string& systemCode = R"(#include "Humble2Core.h"

class {SystemName} final : public HBL2::ISystem
{
public:
	virtual void OnCreate() override
	{
	}

	virtual void OnUpdate(float ts) override
	{
	}
};
/*
// Factory function to create the system
extern "C" __declspec(dllexport) HBL2::ISystem* CreateSystem()
{
	return new {SystemName}();
}*/)";

		size_t pos = systemCode.find(placeholder);

		while (pos != std::string::npos)
		{
			((std::string&)systemCode).replace(pos, placeholder.length(), systemName);
			pos = systemCode.find(placeholder, pos + systemName.length());
		}

		return systemCode;
	}

	std::string NativeScriptUtilities::GetDefaultSolutionText(const std::string& systemName)
	{
		const std::string& solutionText = R"(
Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 17
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "UnityBuild", "UnityBuild.vcxproj", "{0EF03882-FAA7-7ACF-63AF-532B4F8615C0}"
EndProject
Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
		Debug|x64 = Debug|x64
		Release|x64 = Release|x64
	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
		{0EF03882-FAA7-7ACF-63AF-532B4F8615C0}.Debug|x64.ActiveCfg = Debug|x64
		{0EF03882-FAA7-7ACF-63AF-532B4F8615C0}.Debug|x64.Build.0 = Debug|x64
		{0EF03882-FAA7-7ACF-63AF-532B4F8615C0}.Release|x64.ActiveCfg = Release|x64
		{0EF03882-FAA7-7ACF-63AF-532B4F8615C0}.Release|x64.Build.0 = Release|x64
	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
EndGlobal
		)";

		return solutionText;
	}

	std::string NativeScriptUtilities::GetDefaultProjectText(const std::string& projectIncludes)
	{
		const std::string& vulkanSDK = std::getenv("VULKAN_SDK");

		const std::string& placeholderIncludes = "{Includes}";
		const std::string& placeholderPDB = "{randomPDB}";
		const std::string& placeholderVulkan = "{VULKAN_SDK}";
		const std::string& placeholderScene = "{Scene}";
		const std::string& placeholderProject = "{Project}";

		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		const std::string& projectText = R"(<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Debug|x64">
      <Configuration>Debug</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|x64">
      <Configuration>Release</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <ProjectGuid>{0EF03882-FAA7-7ACF-63AF-532B4F8615C0}</ProjectGuid>
    <IgnoreWarnCompileDuplicatedFilename>true</IgnoreWarnCompileDuplicatedFilename>
    <Keyword>Win32Proj</Keyword>
    <RootNamespace>UnityBuild</RootNamespace>
    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'" Label="Configuration">
    <ConfigurationType>DynamicLibrary</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <CharacterSet>Unicode</CharacterSet>
    <PlatformToolset>v143</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'" Label="Configuration">
    <ConfigurationType>DynamicLibrary</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <CharacterSet>Unicode</CharacterSet>
    <PlatformToolset>v143</PlatformToolset>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <ImportGroup Label="ExtensionSettings">
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" + std::string(")") +  R"(" Label ="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" + std::string(")") +  R"(" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <PropertyGroup Label="UserMacros" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <LinkIncremental>true</LinkIncremental>
    <OutDir>..\..\..\assets\dlls\Debug-x86_64\{Project}\{Scene}\</OutDir>
    <IntDir>..\..\..\assets\dlls-int\Debug-x86_64\{Project}\{Scene}\</IntDir>
    <TargetName>UnityBuild</TargetName>
    <TargetExt>.dll</TargetExt>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <LinkIncremental>false</LinkIncremental>
    <OutDir>..\..\..\assets\dlls\Release-x86_64\{Project}\{Scene}\</OutDir>
    <IntDir>..\..\..\assets\dlls-int\Release-x86_64\{Project}\{Scene}\</IntDir>
    <TargetName>UnityBuild</TargetName>
    <TargetExt>.dll</TargetExt>
  </PropertyGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <ClCompile>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <PreprocessorDefinitions>_CRT_SECURE_NO_WARNINGS;YAML_CPP_STATIC_DEFINE;HBL2_PLATFORM_WINDOWS;DEBUG;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>..\..\Assets;..\..\..\..\Humble2\src;..\..\..\..\Humble2\src\Humble2;..\..\..\..\Humble2\src\Vendor;..\..\..\..\Humble2\src\Vendor\spdlog-1.x\include;..\..\..\..\Humble2\src\Vendor\entt\include;..\..\..\..\Dependencies\GLFW\include;..\..\..\..\Dependencies\GLEW\include;..\..\..\..\Dependencies\ImGui\imgui;..\..\..\..\Dependencies\ImGui\imgui\backends;..\..\..\..\Dependencies\ImGuizmo;..\..\..\..\Dependencies\GLM;..\..\..\..\Dependencies\YAML-Cpp\yaml-cpp\include;..\..\..\..\Dependencies\Emscripten\emsdk\upstream\emscripten\system\include;{VULKAN_SDK}\Include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <DebugInformationFormat>EditAndContinue</DebugInformationFormat>
      <Optimization>Disabled</Optimization>
      <MinimalRebuild>false</MinimalRebuild>
      <RuntimeLibrary>MultiThreadedDebugDLL</RuntimeLibrary>
      <MultiProcessorCompilation>true</MultiProcessorCompilation>
      <LanguageStandard>stdcpp20</LanguageStandard>
      <ExternalWarningLevel>Level3</ExternalWarningLevel>
    </ClCompile>
    <Link>
      <SubSystem>Windows</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
      <AdditionalDependencies>Humble2.lib;%(AdditionalDependencies)</AdditionalDependencies>
      <AdditionalLibraryDirectories>..\..\..\..\bin\Debug-x86_64\Humble2;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
      <ImportLibrary>..\..\..\assets\dlls\Debug-x86_64\{Project}\{Scene}\UnityBuild.lib</ImportLibrary>
      <ProgramDatabaseFile>..\..\..\assets\dlls\Debug-x86_64\{Project}\{Scene}\{randomPDB}.pdb</ProgramDatabaseFile>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ClCompile>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <PreprocessorDefinitions>_CRT_SECURE_NO_WARNINGS;YAML_CPP_STATIC_DEFINE;HBL2_PLATFORM_WINDOWS;RELEASE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>..\..\Assets;..\..\..\..\Humble2\src;..\..\..\..\Humble2\src\Humble2;..\..\..\..\Humble2\src\Vendor;..\..\..\..\Humble2\src\Vendor\spdlog-1.x\include;..\..\..\..\Humble2\src\Vendor\entt\include;..\..\..\..\Dependencies\GLFW\include;..\..\..\..\Dependencies\GLEW\include;..\..\..\..\Dependencies\ImGui\imgui;..\..\..\..\Dependencies\ImGui\imgui\backends;..\..\..\..\Dependencies\ImGuizmo;..\..\..\..\Dependencies\GLM;..\..\..\..\Dependencies\YAML-Cpp\yaml-cpp\include;..\..\..\..\Dependencies\Emscripten\emsdk\upstream\emscripten\system\include;{VULKAN_SDK}\Include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <Optimization>Full</Optimization>
      <FunctionLevelLinking>true</FunctionLevelLinking>
      <IntrinsicFunctions>true</IntrinsicFunctions>
      <MinimalRebuild>false</MinimalRebuild>
      <StringPooling>true</StringPooling>
      <RuntimeLibrary>MultiThreadedDLL</RuntimeLibrary>
      <MultiProcessorCompilation>true</MultiProcessorCompilation>
      <LanguageStandard>stdcpp20</LanguageStandard>
      <ExternalWarningLevel>Level3</ExternalWarningLevel>
    </ClCompile>
    <Link>
      <SubSystem>Windows</SubSystem>
      <EnableCOMDATFolding>true</EnableCOMDATFolding>
      <OptimizeReferences>true</OptimizeReferences>
      <AdditionalDependencies>Humble2.lib;%(AdditionalDependencies)</AdditionalDependencies>
      <AdditionalLibraryDirectories>..\..\..\..\bin\Release-x86_64\Humble2;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
      <ImportLibrary>..\..\..\assets\dlls\Release-x86_64\{Project}\{Scene}\UnityBuild.lib</ImportLibrary>
      <ProgramDatabaseFile>..\..\..\assets\dlls\Release-x86_64\{Project}\{Scene}\{randomPDB}.pdb</ProgramDatabaseFile>
    </Link>
  </ItemDefinitionGroup>
  <ItemGroup>
    {Includes}
  </ItemGroup>
  <ItemGroup>
    <ClCompile Include="UnityBuildSource.cpp" />
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
  <ImportGroup Label="ExtensionTargets">
  </ImportGroup>
</Project>
		)";

		// Fill in project includes
		size_t pos = projectText.find(placeholderIncludes);

		while (pos != std::string::npos)
		{
			((std::string&)projectText).replace(pos, placeholderIncludes.length(), projectIncludes);
			pos = projectText.find(placeholderIncludes, pos + projectIncludes.length());
		}

		// Fill in project name
		pos = projectText.find(placeholderProject);
		const std::string& projectName = Project::GetActive()->GetName();

		while (pos != std::string::npos)
		{
			((std::string&)projectText).replace(pos, placeholderProject.length(), projectName);
			pos = projectText.find(placeholderProject, pos + projectName.length());
		}

		// Fill in scene name
		pos = projectText.find(placeholderScene);
		const std::string& sceneName = activeScene->GetName();

		while (pos != std::string::npos)
		{
			((std::string&)projectText).replace(pos, placeholderScene.length(), sceneName);
			pos = projectText.find(placeholderScene, pos + sceneName.length());
		}

		// Fill in pdb file name
		pos = projectText.find(placeholderPDB);
		const std::string& hash = std::to_string(Random::UInt64());

		while (pos != std::string::npos)
		{
			((std::string&)projectText).replace(pos, placeholderPDB.length(), hash);
			pos = projectText.find(placeholderPDB, pos + hash.length());
		}

		// Fill in vulkan SDK path
		pos = projectText.find(placeholderVulkan);

		while (pos != std::string::npos)
		{
			((std::string&)projectText).replace(pos, placeholderVulkan.length(), vulkanSDK);
			pos = projectText.find(placeholderVulkan, pos + vulkanSDK.length());
		}

		return projectText;
	}

	std::string NativeScriptUtilities::GetDefaultComponentCode(const std::string& componentName)
	{
		const std::string& placeholder = "{ComponentName}";

		const std::string& componentCode = R"(#include "Humble2Core.h"

struct {ComponentName}
{
	int Value = 0;
};

// Factory function to register the component
extern "C" __declspec(dllexport) const char* RegisterComponent(entt::meta_ctx* meta_ctx)
{
	using namespace entt::literals;

    // Register the component in the shared reflection context
	entt::meta<{ComponentName}>(*meta_ctx)
		.type(entt::hashed_string(typeid({ComponentName}).name()))
		.data<&{ComponentName}::Value>("Value"_hs).prop("name"_hs, "Value");

	return typeid({ComponentName}).name();
}

// Factory function to add the component
extern "C" __declspec(dllexport) entt::meta_any AddNewComponent(HBL2::Scene* ctx, entt::entity entity)
{
	auto& component = ctx->AddComponent<{ComponentName}>(entity);
	return entt::forward_as_meta(ctx->GetMetaContext(), component);
}

// Factory function to add the component
extern "C" __declspec(dllexport) entt::meta_any GetNewComponent(HBL2::Scene* ctx, entt::entity entity)
{
	auto& component = ctx->GetComponent<{ComponentName}>(entity);
	return entt::forward_as_meta(ctx->GetMetaContext(), component);
}

// Factory function to add the component
extern "C" __declspec(dllexport) bool HasNewComponent(HBL2::Scene* ctx, entt::entity entity)
{
	return ctx->HasComponent<{ComponentName}>(entity);
}
)";

		size_t pos = componentCode.find(placeholder);

		while (pos != std::string::npos)
		{
			((std::string&)componentCode).replace(pos, placeholder.length(), componentName);
			pos = componentCode.find(placeholder, pos + componentName.length());
		}

		return componentCode;
	}

	void NativeScriptUtilities::GenerateComponent(const std::string& componentName)
	{
		// Create component file
		std::ofstream systemFile(HBL2::Project::GetAssetDirectory() / "Scripts" / (componentName + ".cpp"), 0);
		systemFile << NativeScriptUtilities::Get().GetDefaultComponentCode(componentName);
		systemFile.close();

		// Create folder in ProjectFiles
		const auto& projectFilesPath = HBL2::Project::GetAssetDirectory().parent_path() / "ProjectFiles" / componentName;

		try
		{
			std::filesystem::create_directories(projectFilesPath);
		}
		catch (std::exception& e)
		{
			HBL2_ERROR("Project directory creation failed: {0}", e.what());
		}

		// Create solution file for new system
		std::ofstream solutionFile(projectFilesPath / (componentName + ".sln"));

		if (!solutionFile.is_open())
		{
			return;
		}

		solutionFile << NativeScriptUtilities::Get().GetDefaultSolutionText(componentName);
		solutionFile.close();

		// Create vcxproj file for new system
		std::ofstream projectFile(projectFilesPath / (componentName + ".vcxproj"));

		if (!projectFile.is_open())
		{
			return;
		}

		projectFile << NativeScriptUtilities::Get().GetDefaultProjectText(componentName);
		projectFile.close();

		// Build the solution					
#ifdef DEBUG
		const std::string& command = R"(""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe" )" + std::filesystem::path(projectFilesPath / (componentName + ".sln")).string() + R"( /t:)" + componentName + R"( /p:Configuration=Debug")";
#else
		const std::string& command = R"(""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe" )" + std::filesystem::path(projectFilesPath / (componentName + ".sln")).string() + R"( /t:)" + componentName + R"( /p:Configuration=Release")";
#endif // DEBUG
		system(command.c_str());

		Scene* activeScene = ResourceManager::Instance->GetScene(Context::ActiveScene);

		// Load system dll
		const std::string& dllPath = "assets\\dlls\\" + componentName + "\\" + componentName + ".dll";
		NativeScriptUtilities::Get().LoadComponent(dllPath, activeScene);
	}

	void NativeScriptUtilities::CompileComponent(const std::string& componentName)
	{
	}

	ISystem* NativeScriptUtilities::LoadSystem(const std::string& path, Scene* ctx)
	{
		// Load new system dll.
		DynamicLibrary newSystem = DynamicLibrary(path);

		const std::string& name = std::filesystem::path(path).filename().stem().string() + ctx->GetName();

		// Retrieve function that creates the system from the dll.
		CreateSystemFunc createSystem = newSystem.GetFunction<CreateSystemFunc>("CreateSystem");

		// Create the system
		ISystem* system = createSystem();

		if (system != nullptr)
		{
			// Set system name
			system->Name = std::filesystem::path(path).filename().stem().string();

			m_DynamicLibraries[name] = newSystem;

			ctx->RegisterSystem(system, SystemType::User);

			return system;
		}
		
		HBL2_CORE_ERROR("Failed to load system: {0}.", name);

		return nullptr;
	}

	void NativeScriptUtilities::UnloadSystem(const std::string& dllName, Scene* ctx)
	{
		const std::string& fullDllName = dllName + ctx->GetName();

		if (m_DynamicLibraries.find(fullDllName) != m_DynamicLibraries.end())
		{
			m_DynamicLibraries[fullDllName].Free();
			m_DynamicLibraries.erase(fullDllName);
		}
	}

	void NativeScriptUtilities::LoadUnityBuild(Scene* ctx)
	{
		const std::string& projectName = Project::GetActive()->GetName();

#ifdef DEBUG
		const auto& path = std::filesystem::path("assets") / "dlls" / "Debug-x86_64" / projectName / ctx->GetName() / "UnityBuild.dll";
#else
		const auto& path = std::filesystem::path("assets") / "dlls" / "Release-x86_64" / projectName / ctx->GetName() / "UnityBuild.dll";
#endif

		LoadUnityBuild(ctx, path.string());
	}

	void NativeScriptUtilities::LoadUnityBuild(Scene* ctx, const std::string& path)
	{
		// Load new unity build dll.
		DynamicLibrary unityBuild = DynamicLibrary(path);

		// Retrieve function that registers the systems from the dll.
		RegisterSystemsFunc registerSystems = unityBuild.GetFunction<RegisterSystemsFunc>("RegisterSystems");

		// Invoke the register systems func.
		registerSystems(ctx);

		m_DynamicLibraries[ctx->GetName() + "_UnityBuild"] = unityBuild;
	}

	void NativeScriptUtilities::UnloadUnityBuild(Scene* ctx)
	{
		const std::string& fullDllName = ctx->GetName() + "_UnityBuild";

		// Deregister systems.
		for (ISystem* userSystem : ctx->GetRuntimeSystems())
		{
			if (userSystem->GetType() == SystemType::User)
			{
				ctx->DeregisterSystem(userSystem);
			}
		}

		// Free dll and remove from map.
		if (m_DynamicLibraries.find(fullDllName) != m_DynamicLibraries.end())
		{
			m_DynamicLibraries[fullDllName].Free();
			m_DynamicLibraries.erase(fullDllName);
		}
	}

	void NativeScriptUtilities::LoadComponent(const std::string& path, Scene* ctx)
	{
		// Load new component dll.
		DynamicLibrary newComponent(path);

		// Retrieve function that registers the component from the dll.
		RegisterComponentFunc registerComponent = newComponent.GetFunction<RegisterComponentFunc>("RegisterComponent");

		// Register the component.
		const char* name = registerComponent(&ctx->GetMetaContext());

		m_DynamicLibraries[name] = newComponent;
	}

	entt::meta_any NativeScriptUtilities::AddComponent(const std::string& name, Scene* ctx, entt::entity entity)
	{
		// Load new component dll.
		DynamicLibrary& component = m_DynamicLibraries[name];

		// Retrieve function that registers the component from the dll.
		AddNewComponentFunc addComponent = component.GetFunction<AddNewComponentFunc>("AddNewComponent");

		// Register the component.
		return addComponent(ctx, entity);
	}

	entt::meta_any NativeScriptUtilities::GetComponent(const std::string& name, Scene* ctx, entt::entity entity)
	{
		// Load new component dll.
		DynamicLibrary& component = m_DynamicLibraries[name];

		// Retrieve function that registers the component from the dll.
		GetNewComponentFunc getComponent = component.GetFunction<GetNewComponentFunc>("GetNewComponent");

		// Register the component.
		return getComponent(ctx, entity);
	}

	bool NativeScriptUtilities::HasComponent(const std::string& name, Scene* ctx, entt::entity entity)
	{
		// Load new component dll.
		DynamicLibrary& component = m_DynamicLibraries[name];

		// Retrieve function that registers the component from the dll.
		HasNewComponentFunc hasComponent = component.GetFunction<HasNewComponentFunc>("HasNewComponent");

		// Register the component.
		return hasComponent(ctx, entity);
	}
	
	std::string NativeScriptUtilities::CleanComponentName(const std::string& input)
	{
		size_t pos = input.find('>');
		if (pos != std::string::npos)
		{
			return input.substr(0, pos);
		}
		return input;
	}
}
