project "HumbleApp"
    kind "WindowedApp"
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
        "../Humble2/src",
        "../Humble2/src/Humble2",
    }
    
    externalincludedirs
    {
        "../Humble2/src/Vendor/spdlog-1.x/include",
        "../Humble2/src/Vendor/fastgltf/include",
        "../Humble2/src/Vendor",
        "../Dependencies/GLFW/include",
        "../Dependencies/GLEW/include",
        "../Dependencies/ImGui/imgui",
        "../Dependencies/ImGui/imgui/backends",
        "../Dependencies/ImGuizmo/src",
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
            "-rpath @executable_path/../Frameworks"
        }

        xcodebuildsettings
        {
            ["GENERATE_INFOPLIST_FILE"] = "YES",
        }

        postbuildcommands
        {
            "{MKDIR} %{cfg.targetdir}/%{prj.name}.app/Contents/Frameworks",

            -- Humble2
            "{COPY} %{wks.location}/bin/" .. outputdir .. "/Humble2/libHumble2.dylib %{cfg.targetdir}/%{prj.name}.app/Contents/Frameworks/",

            -- Third party
            "{COPY} %{wks.location}/Dependencies/FMOD/MacOS/core/lib/libfmodL.dylib %{cfg.targetdir}/%{prj.name}.app/Contents/Frameworks/",
            "{COPY} %{wks.location}/Dependencies/SLang/slang-2026.11-macos-aarch64/lib/libslang.dylib %{cfg.targetdir}/%{prj.name}.app/Contents/Frameworks/",
            "{COPY} %{wks.location}/Dependencies/SLang/slang-2026.11-macos-aarch64/lib/libslang-compiler.dylib %{cfg.targetdir}/%{prj.name}.app/Contents/Frameworks/",
            "{COPY} %{wks.location}/Dependencies/SLang/slang-2026.11-macos-aarch64/lib/libslang-compiler.0.2026.11.dylib %{cfg.targetdir}/%{prj.name}.app/Contents/Frameworks/",  
            "{COPY} %{wks.location}/Dependencies/SLang/slang-2026.11-macos-aarch64/lib/libslang-glslang-2026.11.dylib %{cfg.targetdir}/%{prj.name}.app/Contents/Frameworks/",

            -- Vulkan
            "{COPY} %{VULKAN_SDK}/lib/libvulkan_kosmickrisp.dylib %{cfg.targetdir}/%{prj.name}.app/Contents/Frameworks/",
            "{COPY} %{VULKAN_SDK}/lib/libvulkan.1.dylib %{cfg.targetdir}/%{prj.name}.app/Contents/Frameworks/",
            "{COPY} %{VULKAN_SDK}/lib/libvulkan.%{VULKAN_SDK_VERSION}.dylib %{cfg.targetdir}/%{prj.name}.app/Contents/Frameworks/",

            "{MKDIR} %{cfg.targetdir}/%{prj.name}.app/Contents/Resources/vulkan/icd.d",
            "{COPY} %{VULKAN_SDK}/share/vulkan/icd.d/libkosmickrisp_icd.json %{cfg.targetdir}/%{prj.name}.app/Contents/Resources/vulkan/icd.d/",
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