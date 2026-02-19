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
        "../Humble2/src/Vendor",
        "../Humble2/src/Vendor/entt/include",
        "../Humble2/src/Vendor/spdlog-1.x/include",
        "../Humble2/src/Vendor/fastgltf/include",
        "../Dependencies/ImGui/imgui",
        "../Dependencies/ImGui/imgui/backends",
        "../Dependencies/ImGuizmo",
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
        "%{VULKAN_SDK}/Include"
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
        table.unpack(JoltDefines)
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

    filter "configurations:Dist"
        defines { "DIST" }
        runtime "Release"
        optimize "Full"
        symbols "Off"

    filter "configurations:Emscripten"
        defines { "EMSCRIPTEN", "__EMSCRIPTEN__" }
        runtime "Release"
        optimize "On"
