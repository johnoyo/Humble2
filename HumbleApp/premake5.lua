project "HumbleApp"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

    files 
    { 
        "src/**.h", 
        "src/**.cpp"
    }

    includedirs
    {
        "../Humble2/src",
        "../Humble2/src/Humble2",
        "../Humble2/src/Vendor",
        "../Humble2/src/Vendor/spdlog-1.x/include",
        "../Humble2/src/Vendor/entt/include",
        "../Dependencies/GLFW/include",
        "../Dependencies/GLEW/include",
        "../Dependencies/ImGui/imgui",
        "../Dependencies/ImGui/imgui/backends",
        "../Dependencies/GLM",
        "../Dependencies/YAML-Cpp/yaml-cpp/include",
        "../Dependencies/Emscripten/emsdk/upstream/emscripten/system/include"
    }

    links
    {
        "Humble2"
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
        defines { "EMSCRIPTEN", "__EMSCRIPTEN__" }
        runtime "Release"
        optimize "On"