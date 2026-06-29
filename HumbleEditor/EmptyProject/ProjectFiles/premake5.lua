VULKAN_SDK = os.getenv("VULKAN_SDK")

workspace "UnityBuildNew"
    architecture "x64"

    configurations 
    { 
        "Debug",
        "Release",
    }

-- Variable to hold output directory.
outputdir = "%{cfg.buildcfg}-%{cfg.architecture}"

-- Engine project.
project "UnityBuildNew"
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
    multiprocessorcompile "On"

    -- Directories for binary and intermediate files.
    targetdir ("../../assets/dlls/" .. outputdir .. "/%{prj.name}")
    objdir ("../../assets/dlls-int/" .. outputdir .. "/%{prj.name}")

    files 
    {
        "../Assets/**.h",
        "UnityBuildSource.cpp",
    }

    defines
	{
		"_CRT_SECURE_NO_WARNINGS",
        "YAML_CPP_STATIC_DEFINE",
	}
    
    -- Include directories.
    includedirs
    {
        "../Assets",
        "../../../Humble2/src",
        "../../../Humble2/src/Humble2",
    }

    externalincludedirs
    {
        "../../../Humble2/src/Vendor",
        "../../../Humble2/src/Vendor/spdlog-1.x/include",
        "../../../Humble2/src/Vendor/fastgltf/include",
        "../../../Dependencies/GLFW/include",
        "../../../Dependencies/GLEW/include",
        "../../../Dependencies/ImGui/imgui",
        "../../../Dependencies/ImGui/imgui/backends",
        "../../../Dependencies/ImGuizmo",
        "../../../Dependencies/GLM",
        "../../../Dependencies/YAML-Cpp/yaml-cpp/include",
        "../../../Dependencies/PortableFileDialogs",
        "../../../Dependencies/FMOD/core/include",
        "../../../Dependencies/Box2D/box2d/include",
        "../../../Dependencies/Box2D/box2d/src",
        "%{VULKAN_SDK}/Include",
        "/Users/johnpetr/VulkanSDK/1.4.350.1/macOS/include",
    }
    
    links
    {
        "Humble2",
    }
    
    filter "system:windows"
        systemversion "latest"    
        defines { "HBL2_PLATFORM_WINDOWS" }
        symbolsfile ("../../assets/dlls/" .. outputdir .. "/%{prj.name}/{pdbName}.pdb")

    filter "system:macosx"
        systemversion "latest"    
        defines { "HBL2_PLATFORM_MACOS" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        staticruntime "Off"
        symbols "On"

    filter "configurations:Release"
        defines { "RELEASE" }
        runtime "Release"
        staticruntime "Off"
        optimize "On"
