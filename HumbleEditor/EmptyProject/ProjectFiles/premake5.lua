VULKAN_SDK = os.getenv("VULKAN_SDK")

workspace "UnityBuild"
    architecture "x64"

    configurations 
    { 
        "Debug",
        "Release",
    }

-- Variable to hold output directory.
outputdir = "%{cfg.buildcfg}-%{cfg.architecture}"

-- Engine project.
project "UnityBuild"
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"

    flags { "MultiProcessorCompile" }

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
        "YAML_CPP_STATIC_DEFINE"
	}
    
    -- Include directories.
    includedirs
    {
        "../Assets",
        "../../../Humble2/src",
        "../../../Humble2/src/Humble2",
        "../../../Humble2/src/Vendor",
        "../../../Humble2/src/Vendor/spdlog-1.x/include",
        "../../../Humble2/src/Vendor/entt/include",
        "../../../Dependencies/GLFW/include",
        "../../../Dependencies/GLEW/include",
        "../../../Dependencies/ImGui/imgui",
        "../../../Dependencies/ImGui/imgui/backends",
        "../../../Dependencies/ImGuizmo",
        "../../../Dependencies/GLM",
        "../../../Dependencies/YAML-Cpp/yaml-cpp/include",
        "../../../Dependencies/Emscripten/emsdk/upstream/emscripten/system/include",
        "%{VULKAN_SDK}/Include"
    }
    
    links
    {
        "Humble2",
        "ImGui",
        "YAML-Cpp"
    }
    
    filter "system:windows"
    systemversion "latest"    
        defines { "HBL2_PLATFORM_WINDOWS" }

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "RELEASE" }
        runtime "Release"
        optimize "On"
        
    filter "configurations:Emscripten"
        defines { "EMSCRIPTEN" }
        runtime "Release"
        optimize "on"
