#include "NativeScriptUtilities.h"

namespace HBL2
{
	extern "C"
	{
		typedef ISystem* (*CreateSystemFunc)();
		
		typedef const char* (*RegisterComponentFunc)();

		typedef entt::meta_any (*AddNewComponentFunc)(Scene*);

		typedef entt::meta_any (*GetNewComponentFunc)(Scene*, entt::entity);
	}

	NativeScriptUtilities* NativeScriptUtilities::s_Instance = nullptr;

	NativeScriptUtilities& NativeScriptUtilities::Get()
	{
		HBL2_CORE_ASSERT(s_Instance != nullptr, "MeshUtilities::s_Instance is null! Call MeshUtilities::Initialize before use.");
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

// Factory function to create the system
extern "C" __declspec(dllexport) HBL2::ISystem* CreateSystem()
{
	return new {SystemName}();
})";

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
		const std::string& placeholder = "{SystemName}";

		const std::string& solutionText = R"(Microsoft Visual Studio Solution File, Format Version 12.00
# Visual Studio Version 17
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "{SystemName}", "{SystemName}.vcxproj", "{999790BD-0504-4CB8-CEF7-E3153A236E20}"
EndProject
Global
	GlobalSection(SolutionConfigurationPlatforms) = preSolution
		Debug|x64 = Debug|x64
		Release|x64 = Release|x64
	EndGlobalSection
	GlobalSection(ProjectConfigurationPlatforms) = postSolution
		{999790BD-0504-4CB8-CEF7-E3153A236E20}.Debug|x64.ActiveCfg = Debug|x64
		{999790BD-0504-4CB8-CEF7-E3153A236E20}.Debug|x64.Build.0 = Debug|x64
		{999790BD-0504-4CB8-CEF7-E3153A236E20}.Release|x64.ActiveCfg = Release|x64
		{999790BD-0504-4CB8-CEF7-E3153A236E20}.Release|x64.Build.0 = Release|x64
	EndGlobalSection
	GlobalSection(SolutionProperties) = preSolution
		HideSolutionNode = FALSE
	EndGlobalSection
EndGlobal
		)";

		size_t pos = solutionText.find(placeholder);

		while (pos != std::string::npos)
		{
			((std::string&)solutionText).replace(pos, placeholder.length(), systemName);
			pos = solutionText.find(placeholder, pos + systemName.length());
		}

		return solutionText;
	}

	std::string NativeScriptUtilities::GetDefaultProjectText(const std::string& systemName)
	{
		const std::string& vulkanSDK = std::getenv("VULKAN_SDK");

		const std::string& placeholder = "{SystemName}";
		const std::string& placeholderPDB = "{randomPDB}";
		const std::string& placeholderVulkan = "{VULKAN_SDK}";

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
    <ProjectGuid>{999790BD-0504-4CB8-CEF7-E3153A236E20}</ProjectGuid>
    <IgnoreWarnCompileDuplicatedFilename>true</IgnoreWarnCompileDuplicatedFilename>
    <Keyword>Win32Proj</Keyword>
    <RootNamespace>{SystemName}</RootNamespace>
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
    <OutDir>..\..\..\assets\dlls\{SystemName}\</OutDir>
    <IntDir>..\..\..\assets\dlls-int\{SystemName}\Debug\</IntDir>
    <TargetName>{SystemName}</TargetName>
    <TargetExt>.dll</TargetExt>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <LinkIncremental>false</LinkIncremental>
    <OutDir>..\..\..\assets\dlls\{SystemName}\</OutDir>
    <IntDir>..\..\..\assets\dlls-int\{SystemName}\Release\</IntDir>
    <TargetName>{SystemName}</TargetName>
    <TargetExt>.dll</TargetExt>
  </PropertyGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <ClCompile>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <PreprocessorDefinitions>YAML_CPP_STATIC_DEFINE;HBL2_PLATFORM_WINDOWS;DEBUG;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>..\..\..\..\Humble2\src;..\..\..\..\Humble2\src\Humble2;..\..\..\..\Humble2\src\Vendor;..\..\..\..\Humble2\src\Vendor\entt\include;..\..\..\..\Humble2\src\Vendor\spdlog-1.x\include;..\..\..\..\Dependencies\ImGui\imgui;..\..\..\..\Dependencies\ImGui\imgui\backends;..\..\..\..\Dependencies\GLFW\include;..\..\..\..\Dependencies\GLEW\include;..\..\..\..\Dependencies\stb_image;..\..\..\..\Dependencies\GLM;..\..\..\..\Dependencies\YAML-Cpp\yaml-cpp\include;{VULKAN_SDK}\Include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
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
      <ImportLibrary>..\..\..\assets\dlls\{SystemName}\{SystemName}.lib</ImportLibrary>
      <ProgramDatabaseFile>..\..\..\assets\dlls\{SystemName}\{randomPDB}.pdb</ProgramDatabaseFile>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ClCompile>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <PreprocessorDefinitions>YAML_CPP_STATIC_DEFINE;HBL2_PLATFORM_WINDOWS;RELEASE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>..\..\..\..\Humble2\src;..\..\..\..\Humble2\src\Humble2;..\..\..\..\Humble2\src\Vendor;..\..\..\..\Humble2\src\Vendor\entt\include;..\..\..\..\Humble2\src\Vendor\spdlog-1.x\include;..\..\..\..\Dependencies\ImGui\imgui;..\..\..\..\Dependencies\ImGui\imgui\backends;..\..\..\..\Dependencies\GLFW\include;..\..\..\..\Dependencies\GLEW\include;..\..\..\..\Dependencies\stb_image;..\..\..\..\Dependencies\GLM;..\..\..\..\Dependencies\YAML-Cpp\yaml-cpp\include;{VULKAN_SDK}\Include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
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
      <ImportLibrary>..\..\..\assets\dlls\{SystemName}\{SystemName}.lib</ImportLibrary>
      <ProgramDatabaseFile>..\..\..\assets\dlls\{SystemName}\{randomPDB}.pdb</ProgramDatabaseFile>
    </Link>
  </ItemDefinitionGroup>
  <ItemGroup>
    <ClCompile Include="..\..\Assets\Scripts\{SystemName}.cpp" />
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
  <ImportGroup Label="ExtensionTargets">
  </ImportGroup>
</Project>
		)";

		size_t pos = projectText.find(placeholder);

		while (pos != std::string::npos)
		{
			((std::string&)projectText).replace(pos, placeholder.length(), systemName);
			pos = projectText.find(placeholder, pos + systemName.length());
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
			m_DynamicLibraries.erase(dllName);
		}
	}

	void NativeScriptUtilities::LoadComponent(const std::string& path)
	{
		// Load new component dll.
		DynamicLibrary newComponent(path);

		// Retrieve function that registers the component from the dll.
		RegisterComponentFunc registerComponent = newComponent.GetFunction<RegisterComponentFunc>("RegisterComponent");

		// Register the component.
		const char* name = registerComponent();

		m_DynamicLibraries[name] = newComponent;
	}

	entt::meta_any NativeScriptUtilities::AddComponent(const std::string& name, Scene* ctx)
	{
		// Load new component dll.
		DynamicLibrary& component = m_DynamicLibraries[name];

		// Retrieve function that registers the component from the dll.
		AddNewComponentFunc addComponent = component.GetFunction<AddNewComponentFunc>("AddNewComponent");

		// Register the component.
		return addComponent(ctx);
	}

	entt::meta_any NativeScriptUtilities::GetComponent(const std::string& name, Scene* ctx)
	{
		return entt::meta_any();
	}
}
