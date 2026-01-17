#include "BuildEngine.h"

#include "Asset\AssetManager.h"
#include "Project\Project.h"
#include "Scene\ISystem.h"

namespace HBL2
{
	BuildEngine* BuildEngine::Instance = nullptr;

	extern "C"
	{
		typedef void (*RegisterSystemFunc)(Scene*);

		typedef const char* (*RegisterComponentFunc)(Scene*);

		typedef entt::meta_any(*AddComponentFunc)(Scene*, Entity);

		typedef entt::meta_any(*GetComponentFunc)(Scene*, Entity);

		typedef void (*RemoveComponentFunc)(Scene*, Entity);

		typedef bool (*HasComponentFunc)(Scene*, Entity);

		typedef void (*ClearComponentStorageFunc)(Scene*);

		typedef void (*SerializeComponentsFunc)(Scene*, std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>>&, bool);

		typedef void (*DeserializeComponentsFunc)(Scene*, std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>>&);
	}

	void BuildEngine::Initialize()
	{

	}

	void BuildEngine::ShutDown()
	{

	}

	bool BuildEngine::Build()
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

		LoadBuild();

		return true;
	}

	void BuildEngine::Combine()
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

	void BuildEngine::Recompile()
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
		std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>> data;

		// Store all registered meta types.
		for (auto meta_type : entt::resolve(activeScene->GetMetaContext()))
		{
			std::string componentName = meta_type.second.info().name().data();
			componentName = CleanComponentNameO3(componentName);
			userComponentNames.push_back(componentName);

			SerializeComponents(componentName, activeScene, data);
		}

		// Unload unity build dll.
		UnloadBuild(activeScene);

		// Combine all .cpp files in assets in unity build source file.
		Combine();

		// Build unity build source dll.
		Build();

		// Re-register systems.
		for (const auto& userSystemName : userSystemNames)
		{
			RegisterSystem(userSystemName, activeScene);
		}

		// Re-register the components.
		for (const auto& userComponentName : userComponentNames)
		{
			RegisterComponent(userComponentName, activeScene);
			DeserializeComponents(userComponentName, activeScene, data);
		}
	}

	bool BuildEngine::Exists()
	{
		const auto& path = GetUnityBuildPath();
		return std::filesystem::exists(path);
	}

	Handle<Asset> BuildEngine::CreateSystemFile(const std::filesystem::path& currentDir, const std::string& systemName)
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

	Handle<Asset> BuildEngine::CreateComponentFile(const std::filesystem::path& currentDir, const std::string& componentName)
	{
		auto relativePath = std::filesystem::relative(currentDir / (componentName + ".h"), HBL2::Project::GetAssetDirectory());

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
			out << YAML::Key << "Type" << YAML::Value << (uint32_t)ScriptType::COMPONENT;
			out << YAML::EndMap;
			out << YAML::EndMap;
			fout << out.c_str();
			fout.close();
		}

		std::ofstream fout(currentDir / (componentName + ".h"), 0);
		fout << GetDefaultComponentCode(componentName);
		fout.close();

		return scriptAssetHandle;
	}

	Handle<Asset> BuildEngine::CreateHelperScriptFile(const std::filesystem::path& currentDir, const std::string& scriptName)
	{
		auto relativePath = std::filesystem::relative(currentDir / (scriptName + ".h"), HBL2::Project::GetAssetDirectory());

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
			out << YAML::Key << "Type" << YAML::Value << (uint32_t)ScriptType::HELPER_SCRIPT;
			out << YAML::EndMap;
			out << YAML::EndMap;
			fout << out.c_str();
			fout.close();
		}

		std::ofstream fout(currentDir / (scriptName + ".h"), 0);
		fout << GetDefaultHelperScriptCode(scriptName);
		fout.close();

		return scriptAssetHandle;
	}

	const std::filesystem::path BuildEngine::GetUnityBuildPath() const
	{
		const std::string& projectName = Project::GetActive()->GetName();

		#ifdef DEBUG
		return std::filesystem::path("assets") / "dlls" / "Debug-x86_64" / projectName / "UnityBuild.dll";
		#else
		return std::filesystem::path("assets") / "dlls" / "Release-x86_64" / projectName / "UnityBuild.dll";
		#endif
	}

	std::string BuildEngine::GetDefaultSystemCode(const std::string& systemName)
	{
		const std::string& placeholder = "{SystemName}";

		const std::string& systemCode = R"(#pragma once

#include "Humble2Core.h"

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

REGISTER_HBL2_SYSTEM({SystemName})
)";
		size_t pos = systemCode.find(placeholder);

		while (pos != std::string::npos)
		{
			((std::string&)systemCode).replace(pos, placeholder.length(), systemName);
			pos = systemCode.find(placeholder, pos + systemName.length());
		}

		return systemCode;
	}

	std::string BuildEngine::GetDefaultSolutionText()
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

	std::string BuildEngine::GetDefaultProjectText(const std::string& projectIncludes)
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

	std::string BuildEngine::GetDefaultComponentCode(const std::string& componentName)
	{
		const std::string& placeholder = "{ComponentName}";

		const std::string& componentCode = R"(#pragma once

#include "Humble2Core.h"

// Just a POD struct
HBL2_COMPONENT({ComponentName},
{
    int Value = 1;
})

// Register members
REGISTER_HBL2_COMPONENT({ComponentName},
	HBL2_COMPONENT_MEMBER({ComponentName}, Value)
)
)";
		size_t pos = componentCode.find(placeholder);

		while (pos != std::string::npos)
		{
			((std::string&)componentCode).replace(pos, placeholder.length(), componentName);
			pos = componentCode.find(placeholder, pos + componentName.length());
		}

		return componentCode;
	}

	std::string BuildEngine::GetDefaultHelperScriptCode(const std::string& scriptName)
	{
		const std::string& placeholder = "{ScriptName}";

		const std::string& scriptCode = R"(#pragma once

#include "Humble2Core.h"

class {ScriptName}
{
	public:
    private:
};
)";
		size_t pos = scriptCode.find(placeholder);

		while (pos != std::string::npos)
		{
			((std::string&)scriptCode).replace(pos, placeholder.length(), scriptName);
			pos = scriptCode.find(placeholder, pos + scriptName.length());
		}

		return scriptCode;
	}

	void BuildEngine::RegisterSystem(const std::string& name, Scene* ctx)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve if dll is not loaded.
		if (!m_DynamicLibrary.IsLoaded())
		{
			m_DynamicLibrary = DynamicLibrary(path.string());
		}

		// Retrieve function that creates the system from the dll.
		auto registerSystem = m_DynamicLibrary.GetFunction<RegisterSystemFunc>("RegisterSystem_" + name);

		// Create the system
		registerSystem(ctx);
	}

	void BuildEngine::RegisterComponent(const std::string& name, Scene* ctx)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve if dll is not loaded.
		if (!m_DynamicLibrary.IsLoaded())
		{
			m_DynamicLibrary = DynamicLibrary(path.string());
		}

		// Retrieve function that registers the component from the dll.
		auto registerComponent = m_DynamicLibrary.GetFunction<RegisterComponentFunc>("RegisterComponent_" + name);

		// Register the component.
		const char* properName = registerComponent(ctx);
	}

	void BuildEngine::LoadBuild()
	{
		const auto& path = GetUnityBuildPath();
		LoadBuild(path.string());
	}

	void BuildEngine::LoadBuild(const std::string& path)
	{
		// Load new unity build dll.
		m_DynamicLibrary = DynamicLibrary(path);
	}

	void BuildEngine::UnloadBuild(Scene* ctx)
	{
		// Clear user defined components.
		for (auto meta_type : entt::resolve(ctx->GetMetaContext()))
		{
			const auto& alias = meta_type.second.info().name();

			if (alias.size() == 0 || alias.size() >= UINT32_MAX || alias.data() == nullptr)
			{
				continue;
			}

			const std::string& componentName = alias.data();

			const std::string& cleanedComponentName = CleanComponentNameO3(componentName);
			ClearComponentStorage(cleanedComponentName, ctx);
		}

		// Reset reflection system.
		entt::meta_reset(ctx->GetMetaContext());
		ctx->GetRegistry().compact();

		std::vector<ISystem*> systemsToBeDeregistered;

		// Deregister systems.
		for (ISystem* userSystem : ctx->GetRuntimeSystems())
		{
			if (userSystem->GetType() == SystemType::User)
			{
				systemsToBeDeregistered.push_back(userSystem);
			}
		}

		for (const auto& system : systemsToBeDeregistered)
		{
			ctx->DeregisterSystem(system);
		}

		systemsToBeDeregistered.clear();

		// Free dll.
		if (m_DynamicLibrary.IsLoaded())
		{
			m_DynamicLibrary.Free();
		}
	}

	entt::meta_any BuildEngine::AddComponent(const std::string& name, Scene* ctx, Entity entity)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve function that registers the component from the dll.
		auto addComponent = m_DynamicLibrary.GetFunction<AddComponentFunc>("AddComponent_" + name);

		// Register the component.
		return addComponent(ctx, entity);
	}

	entt::meta_any BuildEngine::GetComponent(const std::string& name, Scene* ctx, Entity entity)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve function that gets the component from the dll.
		auto getComponent = m_DynamicLibrary.GetFunction<GetComponentFunc>("GetComponent_" + name);

		// Register the component.
		return getComponent(ctx, entity);
	}

	void BuildEngine::RemoveComponent(const std::string& name, Scene* ctx, Entity entity)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve function that removes the component from the dll.
		auto removeComponent = m_DynamicLibrary.GetFunction<RemoveComponentFunc>("RemoveComponent_" + name);

		// Remove the component.
		removeComponent(ctx, entity);
	}

	bool BuildEngine::HasComponent(const std::string& name, Scene* ctx, Entity entity)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve function that checks if the entity has the component from the dll.
		auto hasComponent = m_DynamicLibrary.GetFunction<HasComponentFunc>("HasComponent_" + name);

		// Register the component.
		return hasComponent(ctx, entity);
	}

	void BuildEngine::ClearComponentStorage(const std::string& name, Scene* ctx)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve function that checks if the entity has the component from the dll.
		auto clearComponentStorage = m_DynamicLibrary.GetFunction<ClearComponentStorageFunc>("ClearComponentStorage_" + name);

		// Register the component.
		clearComponentStorage(ctx);
	}

	void BuildEngine::SerializeComponents(const std::string& name, Scene* ctx, std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>>& data, bool cleanRegistry)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve if dll is not loaded.
		if (!m_DynamicLibrary.IsLoaded())
		{
			m_DynamicLibrary = DynamicLibrary(path.string());
		}

		auto serializeComponents = m_DynamicLibrary.GetFunction<SerializeComponentsFunc>("SerializeComponents_" + name);

		serializeComponents(ctx, data, cleanRegistry);
	}

	void BuildEngine::DeserializeComponents(const std::string& name, Scene* ctx, std::unordered_map<std::string, std::unordered_map<Entity, std::vector<std::byte>>>& data)
	{
		const auto& path = GetUnityBuildPath();

		// Retrieve if dll is not loaded.
		if (!m_DynamicLibrary.IsLoaded())
		{
			m_DynamicLibrary = DynamicLibrary(path.string());
		}

		auto deserializeComponents = m_DynamicLibrary.GetFunction<DeserializeComponentsFunc>("DeserializeComponents_" + name);

		deserializeComponents(ctx, data);
	}

	std::string BuildEngine::CleanComponentNameO1(const std::string& input)
	{
		std::string output = input;

		// Find the '>' character and truncate the string if it exists
		size_t pos = output.find('>');
		if (pos != std::string::npos)
		{
			return output = output.substr(0, pos);
		}

		return output;
	}

	std::string BuildEngine::CleanComponentNameO3(const std::string& input)
	{
		std::string output = input;

		// Remove "struct " if it exists
		const std::string structPrefix = "struct ";
		size_t structPos = output.find(structPrefix);
		if (structPos != std::string::npos)
		{
			output.erase(structPos, structPrefix.length());
		}

		// Remove "class " if it exists
		const std::string classPrefix = "class ";
		size_t classPos = output.find(classPrefix);
		if (classPos != std::string::npos)
		{
			output.erase(classPos, classPrefix.length());
		}

		// TODO: Handle namespaces

		// Find the '>' character and truncate the string if it exists
		size_t pos1 = output.find('>');
		if (pos1 != std::string::npos)
		{
			output = output.substr(0, pos1);
		}

		// Find the '_' character and truncate the string if it exists
		size_t pos2 = output.find('_');
		if (pos2 != std::string::npos)
		{
			return output.substr(0, pos2);
		}

		return output;
	}
}