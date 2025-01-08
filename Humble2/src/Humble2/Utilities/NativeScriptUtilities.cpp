#include "NativeScriptUtilities.h"

namespace HBL2
{
	extern "C"
	{
		typedef ISystem* (*CreateSystemFunc)();
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

	ISystem* NativeScriptUtilities::LoadDLL(const std::string& dllPath)
	{
		HINSTANCE hModule = LoadLibraryA(dllPath.c_str());

		if (!hModule)
		{
			std::cerr << "Failed to load DLL!" << std::endl;
			return nullptr;
		}

		CreateSystemFunc createSystem = (CreateSystemFunc)GetProcAddress(hModule, "CreateSystem");
		if (!createSystem)
		{
			std::cerr << "Failed to find CreateSystem in DLL!" << std::endl;
			FreeLibrary(hModule);
			return nullptr;
		}

		ISystem* system = createSystem();

		// Set name
		system->Name = std::filesystem::path(dllPath).filename().stem().string();

		m_DLLInstances[system->Name] = hModule;

		return system;
	}

	void NativeScriptUtilities::DeleteDLLInstance(const std::string& dllName)
	{
		if (m_DLLInstances.find(dllName) != m_DLLInstances.end())
		{
			FreeLibrary(m_DLLInstances[dllName]);
			// TODO: Investigate why setting m_DLLInstances[dllName] to nullptr or erasing key dllName causes errors to dll.
		}
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
      <PreprocessorDefinitions>GLEW_STATIC;YAML_CPP_STATIC_DEFINE;HBL2_PLATFORM_WINDOWS;DEBUG;%(PreprocessorDefinitions)</PreprocessorDefinitions>
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
      <PreprocessorDefinitions>GLEW_STATIC;YAML_CPP_STATIC_DEFINE;HBL2_PLATFORM_WINDOWS;RELEASE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
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
}
