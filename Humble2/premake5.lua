-- Engine project.
project "Humble2"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"

    -- Directories for binary and intermediate files.
    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

    files 
    { 
        "src/**.h", 
        "src/**.cpp"
    }

    defines
	{
		"_CRT_SECURE_NO_WARNINGS",
        "YAML_CPP_STATIC_DEFINE",
        "GLEW_STATIC",
	}
    
    -- Include directories.
    includedirs
    {
        "src",
        "src/Humble2",
        "src/Vendor",
        "src/Vendor/spdlog-1.x/include",
        "src/Vendor/entt/include",
        "../Dependencies/GLFW/include",
        "../Dependencies/GLEW/include",
        "../Dependencies/ImGui/imgui",
        "../Dependencies/ImGui/imgui/backends",
        "../Dependencies/GLM",
        "../Dependencies/YAML-Cpp/yaml-cpp/include",
        "../Dependencies/Emscripten/emsdk/upstream/emscripten/system/include",
        "%{VULKAN_SDK}/Include"
    }
    
    libdirs
    {
        "../Dependencies/GLFW/lib-vc2022",
        "../Dependencies/GLEW/lib/Release/x64",
        "%{VULKAN_SDK}/Lib"
    }
    
    links
    {
        "glew32s.lib",
        "glfw3.lib",
        "opengl32.lib",

        "vulkan-1.lib",

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

        links
        {
            "shaderc_sharedd.lib",
            "spirv-cross-cored.lib",
            "spirv-cross-glsld.lib",
        }

    filter "configurations:Release"
        defines { "RELEASE" }
        runtime "Release"
        optimize "On"

        links
        {
            "shaderc_shared.lib",
            "spirv-cross-core.lib",
            "spirv-cross-glsl.lib",
        }
        
    filter "configurations:Emscripten"
        defines { "EMSCRIPTEN" }
        runtime "Release"
        optimize "on"

        links
        {
            "shaderc_shared.lib",
            "spirv-cross-core.lib",
            "spirv-cross-glsl.lib",
        }
