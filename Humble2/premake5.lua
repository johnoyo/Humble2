-- Engine project.
project "Humble2"
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"

    flags { "MultiProcessorCompile" }

    -- Directories for binary and intermediate files.
    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

    files 
    { 
        "src/**.h", 
        "src/**.cpp",
    }

    defines
	{
		"_CRT_SECURE_NO_WARNINGS",
        "YAML_CPP_STATIC_DEFINE",

        "HBL_BUILD_DLL"
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
        "../Dependencies/ImGuizmo",
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
        "glew32.lib",
        "glfw3.lib",
        "opengl32.lib",

        "vulkan-1.lib",

        "ImGui",
        "YAML-Cpp"
    }
    
    filter "system:windows"
    systemversion "latest"    
        defines { "HBL2_PLATFORM_WINDOWS" }

        postbuildcommands
        {
            ("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} ../Dependencies/GLEW/bin/Release/x64/glew32.dll ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/GLEW/bin/Release/x64/glew32.dll ../bin/" .. outputdir .. "/HumbleApp")
        }

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
