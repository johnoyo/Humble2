-- Engine project.
project "Humble2"
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"
    multiprocessorcompile "On"

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

        "HBL2_BUILD_DLL",
	}
    
    -- Include directories.
    includedirs
    {
        "src",
        "src/Humble2",
    }
    
    externalincludedirs
    {
        "src/Vendor",
        "../Humble2/src/Vendor/spdlog-1.x/include",
        "../Humble2/src/Vendor/fastgltf/include",
        "../Dependencies/GLFW/include",
        "../Dependencies/GLEW/include",
        "../Dependencies/ImGui/imgui",
        "../Dependencies/ImGui/imgui/backends",
        "../Dependencies/ImGuizmo",
        "../Dependencies/GLM",
        "../Dependencies/YAML-Cpp/yaml-cpp/include",
        "../Dependencies/PortableFileDialogs",
        "../Dependencies/FMOD/include",
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
        "ImGui",
        "YAML-Cpp",
        "Box2D",
        "Jolt",
    }
    
    filter "system:windows"
        systemversion "latest"    
        defines { "HBL2_PLATFORM_WINDOWS", table.unpack(JoltDefines) }

        libdirs
        {
            "../Dependencies/GLFW/glfw-3.4.bin.WIN64/lib-vc2022",
            "../Dependencies/GLEW/lib/Release/x64",
            "../Dependencies/FMOD/Windows/core/lib/x64",
            "../Dependencies/SLang/slang-2026.11-windows-x86_64/lib",
            "%{VULKAN_SDK}/Lib"
        }

        links
        {
            "glew32.lib",
            "glfw3.lib",
            "opengl32.lib",

            "vulkan-1.lib",

            "slang.lib",
            "slang-compiler.lib",
        }

        postbuildcommands
        {
            -- Ensure the HumbleEditor directory exists and copy the target file
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/HumbleEditor"),
            
            -- Ensure the HumbleApp directory exists and copy the target file
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/HumbleApp"),
            
            -- Ensure the GLEW and SLang DLLs are copied to HumbleEditor
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/GLEW/bin/Release/x64/glew32.dll ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-windows-x86_64/bin/slang.dll ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-windows-x86_64/bin/slang-compiler.dll ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-windows-x86_64/bin/slang-glslang.dll ../bin/" .. outputdir .. "/HumbleEditor"),
            
            -- Ensure the GLEW and SLang DLLs are copied to HumbleApp
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} ../Dependencies/GLEW/bin/Release/x64/glew32.dll ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-windows-x86_64/bin/slang.dll ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-windows-x86_64/bin/slang-compiler.dll ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-windows-x86_64/bin/slang-glslang.dll ../bin/" .. outputdir .. "/HumbleApp"),
        }

        filter { "system:windows", "configurations:Debug" }
            defines { "DEBUG" }
            runtime "Debug"
            symbols "On"

            links
            {
                "fmodL_vc.lib",
            }

            postbuildcommands
            {
                -- Ensure the Humble and FMOD DLLs are copied to HumbleEditor
                ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} ../Dependencies/FMOD/Windows/core/lib/x64/fmodL.dll ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} %{cfg.buildtarget.directory}/Humble2.pdb ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} %{cfg.buildtarget.directory}/Humble2.dll ../bin/" .. outputdir .. "/HumbleEditor"),

                -- Ensure the Humble and FMOD DLLs are copied to HumbleApp
                ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} ../Dependencies/FMOD/Windows/core/lib/x64/fmodL.dll ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} %{cfg.buildtarget.directory}/Humble2.pdb ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} %{cfg.buildtarget.directory}/Humble2.dll ../bin/" .. outputdir .. "/HumbleApp"),
            }
            
        filter { "system:windows", "configurations:Release" }
            defines { "RELEASE" }
            runtime "Release"
            optimize "On"
            
            links
            {
                "fmod_vc.lib",
            }

            postbuildcommands
            {
                -- Ensure the Humble and FMOD DLLs are copied to HumbleEditor
                ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} ../Dependencies/FMOD/Windows/core/lib/x64/fmod.dll ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} %{cfg.buildtarget.directory}/Humble2.pdb ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} %{cfg.buildtarget.directory}/Humble2.dll ../bin/" .. outputdir .. "/HumbleEditor"),

                -- Ensure the Humble and FMOD DLLs are copied to HumbleApp
                ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} ../Dependencies/FMOD/Windows/core/lib/x64/fmod.dll ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} %{cfg.buildtarget.directory}/Humble2.pdb ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} %{cfg.buildtarget.directory}/Humble2.dll ../bin/" .. outputdir .. "/HumbleApp"),
            }

        filter { "system:windows", "configurations:Dist" }
            defines { "DIST" }
            runtime "Release"
            optimize "Full"
            symbols "Off"
            
            links
            {
                "fmod_vc.lib",
            }

            postbuildcommands
            {
                -- Ensure the FMOD DLL is copied to HumbleEditor
                ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} ../Dependencies/FMOD/Windows/core/lib/x64/fmod.dll ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} %{cfg.buildtarget.directory}/Humble2.dll ../bin/" .. outputdir .. "/HumbleEditor"),

                -- Ensure the FMOD DLL is copied to HumbleApp
                ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} ../Dependencies/FMOD/Windows/core/lib/x64/fmod.dll ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} %{cfg.buildtarget.directory}/Humble2.dll ../bin/" .. outputdir .. "/HumbleApp"),
            }
            
        filter { "system:windows", "configurations:Emscripten" }
            defines { "EMSCRIPTEN" }
            runtime "Release"
            optimize "On"
            
            links
            {
                "fmod_vc.lib",
            }

            postbuildcommands
            {
                -- Ensure the FMOD DLL is copied to HumbleEditor
                ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} ../Dependencies/FMOD/Windows/core/lib/x64/fmod.dll ../bin/" .. outputdir .. "/HumbleEditor"),

                -- Ensure the FMOD DLL is copied to HumbleApp
                ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} ../Dependencies/FMOD/Windows/core/lib/x64/fmod.dll ../bin/" .. outputdir .. "/HumbleApp"),
            }

    filter "system:macosx"
        systemversion "latest"    
        defines { "HBL2_PLATFORM_MACOS", table.unpack(JoltDefinesArm) }

        files 
        { 
            "src/**.mm",
        }

        removefiles
        {
            "src/Platform/OpenGL/**.h",
            "src/Platform/OpenGL/**.cpp"
        }

        linkoptions
        {
            "-rpath @executable_path",
            "-rpath " .. VULKAN_SDK .. "/lib",

            "-install_name @rpath/libHumble2.dylib",
        }

        libdirs
        {
            "../Dependencies/GLFW/glfw-3.4.bin.MACOS/lib-arm64",
            "../Dependencies/FMOD/MacOS/core/lib",
            "../Dependencies/SLang/slang-2026.11-macos-aarch64/lib",
            "%{VULKAN_SDK}/lib",
        }

        links
        {
            "glfw3",

            -- Vulkan
            "vulkan.1",

            -- Slang
            "slang",
            "slang-compiler",

            -- Required Apple frameworks
            "Cocoa.framework",
            "IOKit.framework",
            "CoreVideo.framework",
            "QuartzCore.framework",
            "Metal.framework",
            "Foundation.framework"
        }

        postbuildcommands
        {
            -- HumbleEditor
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/HumbleEditor"),

            -- Slang dylibs -> HumbleEditor
            ("{COPY} ../Dependencies/SLang/slang-2026.11-macos-aarch64/lib/libslang.dylib ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-macos-aarch64/lib/libslang-compiler.dylib ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-macos-aarch64/lib/libslang-compiler.0.2026.11.dylib ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-macos-aarch64/lib/libslang-glslang-2026.11.dylib ../bin/" .. outputdir .. "/HumbleEditor"),

            ("xattr -dr com.apple.quarantine ../bin"),
        }

        filter { "system:macosx", "configurations:Debug" }
            defines { "DEBUG" }
            runtime "Debug"
            symbols "On"

            links
            {
                "fmodL",
            }

            postbuildcommands
            {
                -- HumbleEditor
                ("{COPY} ../Dependencies/FMOD/MacOS/core/lib/libfmodL.dylib ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} %{cfg.buildtarget.directory}/libHumble2.dylib ../bin/" .. outputdir .. "/HumbleEditor"),

                ("xattr -dr com.apple.quarantine ../bin"),
            }
            
        filter { "system:macosx", "configurations:Release" }
            defines { "RELEASE" }
            runtime "Release"
            optimize "On"
            
            links
            {
                "fmod",
            }

            postbuildcommands
            {
                -- HumbleEditor
                ("{COPY} ../Dependencies/FMOD/MacOS/core/lib/libfmod.dylib ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} %{cfg.buildtarget.directory}/libHumble2.dylib ../bin/" .. outputdir .. "/HumbleEditor"),

                ("xattr -dr com.apple.quarantine ../bin"),
            }

        filter { "system:macosx", "configurations:Dist" }
            defines { "DIST" }
            runtime "Release"
            optimize "Full"
            symbols "Off"
            
            links
            {
                "fmod",
            }

            postbuildcommands
            {
                -- HumbleEditor
                ("{COPY} ../Dependencies/FMOD/MacOS/core/lib/libfmod.dylib ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} %{cfg.buildtarget.directory}/libHumble2.dylib ../bin/" .. outputdir .. "/HumbleEditor"),

                -- HumbleApp
                ("{COPY} ../Dependencies/FMOD/MacOS/core/lib/libfmod.dylib ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} %{cfg.buildtarget.directory}/libHumble2.dylib ../bin/" .. outputdir .. "/HumbleApp"),

                ("xattr -dr com.apple.quarantine ../bin"),
            }

    filter "system:linux"
        systemversion "latest"    
        defines { "HBL2_PLATFORM_LINUX", table.unpack(JoltDefines) }
        buildoptions { "-Wno-changes-meaning", "-march=native" }

        removefiles
        {
            "src/Platform/OpenGL/**.h",
            "src/Platform/OpenGL/**.cpp"
        }

        runpathdirs
        { 
            VULKAN_SDK .. "/lib/VulkanLoader/lib",
            "../Dependencies/SLang/slang-2026.11-linux-x86_64/lib",
            "../Dependencies/FMOD/Linux/core/lib/x86_64"
        }
        
        -- Link-time hints for indirect dependencies
        linkoptions
        {
            "-Wl,-rpath-link=../Dependencies/SLang/slang-2026.11-linux-x86_64/lib:-Wl,-rpath-link=../Dependencies/FMOD/Linux/core/lib/x86_64"
        }

        libdirs
        {
            "../Dependencies/GLFW/glfw-3.4.bin.LINUX/x86_64",
            "../Dependencies/FMOD/Linux/core/lib/x86_64",
            "../Dependencies/SLang/slang-2026.11-linux-x86_64/lib",
            "%{VULKAN_SDK}/lib/VulkanLoader/lib"
        }

        links
        {
            "glfw3",

            "vulkan",

            "slang",
            "slang-compiler",

            "X11",
        }

        postbuildcommands
        {
            -- Ensure the HumbleEditor directory exists and copy the target file
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/HumbleEditor"),
            
            -- Ensure the HumbleApp directory exists and copy the target file
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/HumbleApp"),
            
            -- Ensure the GLEW and SLang DLLs are copied to HumbleEditor
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-linux-x86_64/lib/libslang.so ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-linux-x86_64/lib/libslang-compiler.so ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-linux-x86_64/lib/libslang-compiler.so.0.2026.11 ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-linux-x86_64/lib/libslang-glslang-2026.11.so ../bin/" .. outputdir .. "/HumbleEditor"),
            
            -- Ensure the GLEW and SLang DLLs are copied to HumbleApp
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-linux-x86_64/lib/libslang.so ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-linux-x86_64/lib/libslang-compiler.so ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-linux-x86_64/lib/libslang-compiler.so.0.2026.11 ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} ../Dependencies/SLang/slang-2026.11-linux-x86_64/lib/libslang-glslang-2026.11.so ../bin/" .. outputdir .. "/HumbleApp"),
        }

        filter { "system:linux", "configurations:Debug" }
            defines { "DEBUG" }
            runtime "Debug"
            symbols "On"

            links
            {
                "fmodL",
            }

            postbuildcommands
            {
                -- Ensure the Humble and FMOD DLLs are copied to HumbleEditor
                ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} ../Dependencies/FMOD/Linux/core/lib/x86_64/libfmodL.so ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} ../Dependencies/FMOD/Linux/core/lib/x86_64/libfmodL.so.14 ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} %{cfg.buildtarget.directory}/libHumble2.so ../bin/" .. outputdir .. "/HumbleEditor"),

                -- Ensure the Humble and FMOD DLLs are copied to HumbleApp
                ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} ../Dependencies/FMOD/Linux/core/lib/x86_64/libfmodL.so ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} ../Dependencies/FMOD/Linux/core/lib/x86_64/libfmodL.so.14 ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} %{cfg.buildtarget.directory}/libHumble2.so ../bin/" .. outputdir .. "/HumbleApp"),
            }
            
        filter { "system:linux", "configurations:Release" }
            defines { "RELEASE" }
            runtime "Release"
            optimize "On"
            
            links
            {
                "fmod",
            }

            postbuildcommands
            {
                -- Ensure the Humble and FMOD DLLs are copied to HumbleEditor
                ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} ../Dependencies/FMOD/Linux/core/lib/x86_64/libfmod.so ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} ../Dependencies/FMOD/Linux/core/lib/x86_64/libfmod.so.14 ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} %{cfg.buildtarget.directory}/libHumble2.so ../bin/" .. outputdir .. "/HumbleEditor"),

                -- Ensure the Humble and FMOD DLLs are copied to HumbleApp
                ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} ../Dependencies/FMOD/Linux/core/lib/x86_64/libfmod.so ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} ../Dependencies/FMOD/Linux/core/lib/x86_64/libfmod.so.14 ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} %{cfg.buildtarget.directory}/libHumble2.so ../bin/" .. outputdir .. "/HumbleApp"),
            }

        filter { "system:linux", "configurations:Dist" }
            defines { "DIST" }
            runtime "Release"
            optimize "Full"
            symbols "Off"
            
            links
            {
                "fmod",
            }

            postbuildcommands
            {
                -- Ensure the FMOD DLL is copied to HumbleEditor
                ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} ../Dependencies/FMOD/Linux/core/lib/x86_64/libfmod.so ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} ../Dependencies/FMOD/Linux/core/lib/x86_64/libfmod.so.14 ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} %{cfg.buildtarget.directory}/libHumble2.so ../bin/" .. outputdir .. "/HumbleEditor"),

                -- Ensure the FMOD DLL is copied to HumbleApp
                ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} ../Dependencies/FMOD/Linux/core/lib/x86_64/libfmod.so ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} ../Dependencies/FMOD/Linux/core/lib/x86_64/libfmod.so.14 ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} %{cfg.buildtarget.directory}/libHumble2.so ../bin/" .. outputdir .. "/HumbleApp"),
            }
            
        filter { "system:linux", "configurations:Emscripten" }
            defines { "EMSCRIPTEN" }
            runtime "Release"
            optimize "On"
            
            links
            {
                "fmod",
            }

            postbuildcommands
            {
                -- Ensure the FMOD DLL is copied to HumbleEditor
                ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} ../Dependencies/FMOD/Linux/core/lib/x86_64/libfmod.so ../bin/" .. outputdir .. "/HumbleEditor"),
                ("{COPY} ../Dependencies/FMOD/Linux/core/lib/x86_64/libfmod.so.14 ../bin/" .. outputdir .. "/HumbleEditor"),

                -- Ensure the FMOD DLL is copied to HumbleApp
                ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} ../Dependencies/FMOD/Linux/core/lib/x86_64/libfmod.so ../bin/" .. outputdir .. "/HumbleApp"),
                ("{COPY} ../Dependencies/FMOD/Linux/core/lib/x86_64/libfmod.so.14 ../bin/" .. outputdir .. "/HumbleApp"),
            }
