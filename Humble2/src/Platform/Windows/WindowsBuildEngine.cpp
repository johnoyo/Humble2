#include "WindowsBuildEngine.h"

#include "Project\Project.h"

namespace HBL2
{
	bool WindowsBuildEngine::Build()
	{
		// Combine all .cpp files in assets in unity build source file.
		Combine();

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
		const auto& projectFilesPath = Project::GetAssetDirectory().parent_path() / "ProjectFiles";

		// Build the solution					
#ifdef DEBUG
		const std::string& command = R"(""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe" )" + std::filesystem::path(projectFilesPath / "UnityBuild.sln").string() + R"( /t:)" + "UnityBuild" + R"( /p:Configuration=Debug")";
#else
		const std::string& command = R"(""C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\msbuild.exe" )" + std::filesystem::path(projectFilesPath / "UnityBuild.sln").string() + R"( /t:)" + "UnityBuild" + R"( /p:Configuration=Release")";
#endif // DEBUG

		system(command.c_str());

		LoadBuild();

		return true;
	}

	void WindowsBuildEngine::Combine()
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

		solutionFile << GetDefaultSolutionText();
		solutionFile.close();

		// Create vcxproj file for new system
		std::ofstream projectFile(projectFilesPath / "UnityBuild.vcxproj");

		if (!projectFile.is_open())
		{
			return;
		}

		projectFile << GetDefaultProjectText(projectIncludes);
		projectFile.close();
	}

	std::string WindowsBuildEngine::GetDefaultSolutionText()
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

	std::string WindowsBuildEngine::GetDefaultProjectText(const std::string& projectIncludes)
	{
		const std::string& vulkanSDK = std::getenv("VULKAN_SDK");

		const std::string& placeholderIncludes = "{Includes}";
		const std::string& placeholderPDB = "{randomPDB}";
		const std::string& placeholderVulkan = "{VULKAN_SDK}";
		const std::string& placeholderProject = "{Project}";
		const std::string& placeholderHash = "{Hash}";

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
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" + std::string(")") + R"(" Label ="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" + std::string(")") + R"(" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <PropertyGroup Label="UserMacros" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <LinkIncremental>true</LinkIncremental>
    <OutDir>..\..\assets\dlls\Debug-x86_64\{Project}\</OutDir>
    <IntDir>..\..\assets\dlls-int\Debug-x86_64\{Project}\</IntDir>
    <TargetName>UnityBuild</TargetName>
    <TargetExt>.dll</TargetExt>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <LinkIncremental>false</LinkIncremental>
    <OutDir>..\..\assets\dlls\Release-x86_64\{Project}\</OutDir>
    <IntDir>..\..\assets\dlls-int\Release-x86_64\{Project}\</IntDir>
    <TargetName>UnityBuild</TargetName>
    <TargetExt>.dll</TargetExt>
  </PropertyGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <ClCompile>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <PreprocessorDefinitions>_CRT_SECURE_NO_WARNINGS;YAML_CPP_STATIC_DEFINE;COMPONENT_NAME_HASH={Hash};HBL2_PLATFORM_WINDOWS;DEBUG;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>..\Assets;..\..\..\Humble2\src;..\..\..\Humble2\src\Humble2;..\..\..\Humble2\src\Vendor;..\..\..\Humble2\src\Vendor\spdlog-1.x\include;..\..\..\Humble2\src\Vendor\entt\include;..\..\..\Humble2\src\Vendor\fastgltf\include;..\..\..\Dependencies\GLFW\include;..\..\..\Dependencies\GLEW\include;..\..\..\Dependencies\ImGui\imgui;..\..\..\Dependencies\ImGui\imgui\backends;..\..\..\Dependencies\ImGuizmo;..\..\..\Dependencies\GLM;..\..\..\Dependencies\YAML-Cpp\yaml-cpp\include;..\..\..\Dependencies\PortableFileDialogs;..\..\..\Dependencies\FMOD\core\include;..\..\..\Dependencies\Box2D\box2d\include;..\..\..\Dependencies\Box2D\box2d\src;..\..\..\Dependencies\Emscripten\emsdk\upstream\emscripten\system\include;{VULKAN_SDK}\Include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
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
      <AdditionalLibraryDirectories>..\..\..\bin\Debug-x86_64\Humble2;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
      <ImportLibrary>..\..\assets\dlls\Debug-x86_64\{Project}\UnityBuild.lib</ImportLibrary>
      <ProgramDatabaseFile>..\..\assets\dlls\Debug-x86_64\{Project}\{randomPDB}.pdb</ProgramDatabaseFile>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ClCompile>
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <PreprocessorDefinitions>_CRT_SECURE_NO_WARNINGS;YAML_CPP_STATIC_DEFINE;COMPONENT_NAME_HASH={Hash};HBL2_PLATFORM_WINDOWS;RELEASE;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>..\Assets;..\..\..\Humble2\src;..\..\..\Humble2\src\Humble2;..\..\..\Humble2\src\Vendor;..\..\..\Humble2\src\Vendor\spdlog-1.x\include;..\..\..\Humble2\src\Vendor\entt\include;..\..\..\Humble2\src\Vendor\fastgltf\include;..\..\..\Dependencies\GLFW\include;..\..\..\Dependencies\GLEW\include;..\..\..\Dependencies\ImGui\imgui;..\..\..\Dependencies\ImGui\imgui\backends;..\..\..\Dependencies\ImGuizmo;..\..\..\Dependencies\GLM;..\..\..\Dependencies\YAML-Cpp\yaml-cpp\include;..\..\..\Dependencies\PortableFileDialogs;..\..\..\Dependencies\FMOD\core\include;..\..\..\Dependencies\Box2D\box2d\include;..\..\..\Dependencies\Box2D\box2d\src;..\..\..\Dependencies\Emscripten\emsdk\upstream\emscripten\system\include;{VULKAN_SDK}\Include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
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
      <AdditionalLibraryDirectories>..\..\..\bin\Release-x86_64\Humble2;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
      <ImportLibrary>..\..\assets\dlls\Release-x86_64\{Project}\UnityBuild.lib</ImportLibrary>
      <ProgramDatabaseFile>..\..\assets\dlls\Release-x86_64\{Project}\{randomPDB}.pdb</ProgramDatabaseFile>
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

		// Fill in hash
		pos = projectText.find(placeholderHash);
		const std::string& componentNameHash = std::to_string(Random::UInt64());

		while (pos != std::string::npos)
		{
			((std::string&)projectText).replace(pos, placeholderHash.length(), componentNameHash);
			pos = projectText.find(placeholderHash, pos + componentNameHash.length());
		}

		return projectText;
	}
}