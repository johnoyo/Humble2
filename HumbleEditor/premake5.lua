project "HumbleEditor"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"
    staticruntime "off"

    files { "src/**.h", "src/**.cpp" }

    includedirs
    {
        "src",
        "../Humble2/src",
        "../Humble2/src/Vendor",
        "../Humble2/src/Vendor/entt/include",
        "../Humble2/src/Vendor/spdlog-1.x/include",
        "../Dependencies/ImGui/imgui",
        "../Dependencies/ImGui/imgui/backends",
        "../Dependencies/GLFW/include",
        "../Dependencies/GLEW/include",
        "../Dependencies/stb_image",
        "../Dependencies/GLM"
    }

    links
    {
        "Humble2"
    }

    defines 
    {
        "GLEW_STATIC"
    }

    targetdir ("../bin/" .. outputdir .. "/%{prj.name}")
    objdir ("../bin-int/" .. outputdir .. "/%{prj.name}")

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
        optimize "on"