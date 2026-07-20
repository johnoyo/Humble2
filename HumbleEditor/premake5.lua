project "HumbleEditor"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"
    multiprocessorcompile "On"

    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

    files 
    { 
        "src/**.h", 
        "src/**.cpp",
    }

    includedirs
    {
        "src",
        "../Humble2/src",
        "../Humble2/src/Humble2",
    }
    
    externalincludedirs
    {
        "../Humble2/src/Vendor/spdlog-1.x/include",
        "../Humble2/src/Vendor/fastgltf/include",
        "../Humble2/src/Vendor",
        "../Dependencies/ImGui/imgui",
        "../Dependencies/ImGui/imgui/backends",
        "../Dependencies/ImGuizmo/src",
        "../Dependencies/GLFW/include",
        "../Dependencies/GLEW/include",
        "../Dependencies/stb_image",
        "../Dependencies/GLM",
        "../Dependencies/YAML-Cpp/yaml-cpp/include",
        "../Dependencies/PortableFileDialogs",
        "../Dependencies/FMOD/core/include",
        "../Dependencies/Box2D/box2d/src",
        "../Dependencies/Box2D/box2d/include",
        "../Dependencies/Jolt/jolt",
        "../Dependencies/Emscripten/emsdk/upstream/emscripten/system/include",
        "../Dependencies/SLang/include",
        "%{VULKAN_SDK}/Include",
        "%{VULKAN_SDK}/include",
    }

    links
    {
        "Humble2",
        "ImGui",
        "YAML-Cpp",
        "Box2D",
        "Jolt",
    }

    defines
    {
        "YAML_CPP_STATIC_DEFINE",
    }

    filter "system:windows"
        systemversion "latest"
        defines { "HBL2_PLATFORM_WINDOWS", table.unpack(JoltDefines) }

    filter "system:macosx"
        systemversion "latest"
        defines { "HBL2_PLATFORM_MACOS", table.unpack(JoltDefinesArm) }

        linkoptions
        {
            "-rpath @executable_path"
        }

    filter "system:linux"
        systemversion "latest"
        defines { "HBL2_PLATFORM_LINUX", table.unpack(JoltDefines) }
        buildoptions { "-Wno-changes-meaning", "-march=native" }

        runpathdirs
        { 
            VULKAN_SDK .. "/lib/VulkanLoader/lib",
            "../Dependencies/SLang/slang-2026.11-linux-x86_64/lib",
            "../Dependencies/FMOD/Linux/core/lib/x86_64"
        }
        
        linkoptions
        {
            "-Wl,-rpath-link=../Dependencies/SLang/slang-2026.11-linux-x86_64/lib:-Wl,-rpath-link=../Dependencies/FMOD/Linux/core/lib/x86_64"
        }

        links
        {
            "X11",
        }

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "RELEASE" }
        runtime "Release"
        optimize "On"

    filter "configurations:Dist"
        defines { "DIST" }
        runtime "Release"
        optimize "Full"
        symbols "Off"

    filter "configurations:Emscripten"
        defines { "EMSCRIPTEN", "__EMSCRIPTEN__" }
        runtime "Release"
        optimize "On"
