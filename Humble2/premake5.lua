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

        table.unpack(JoltDefines)
	}
    
    -- Include directories.
    includedirs
    {
        "src",
        "src/Humble2",
        "src/Vendor",
        "src/Vendor/spdlog-1.x/include",
        "src/Vendor/entt/include",
        "src/Vendor/fastgltf/include",
        "../Dependencies/GLFW/include",
        "../Dependencies/GLEW/include",
        "../Dependencies/ImGui/imgui",
        "../Dependencies/ImGui/imgui/backends",
        "../Dependencies/ImGuizmo",
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
    
    libdirs
    {
        "../Dependencies/GLFW/lib-vc2022",
        "../Dependencies/GLEW/lib/Release/x64",
        "../Dependencies/FMOD/core/lib/x64",
        "%{VULKAN_SDK}/Lib"
    }
    
    links
    {
        "glew32.lib",
        "glfw3.lib",
        "opengl32.lib",

        "vulkan-1.lib",

        "ImGui",
        "YAML-Cpp",
        "Box2D",
        "Jolt",
    }
    
    defines (JoltDefines)

    filter "system:windows"
        systemversion "latest"    
        defines { "HBL2_PLATFORM_WINDOWS" }

        postbuildcommands
        {
            -- Ensure the HumbleEditor directory exists and copy the target file
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/HumbleEditor"),
            
            -- Ensure the HumbleApp directory exists and copy the target file
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputdir .. "/HumbleApp"),
            
            -- Ensure the GLEW and FMOD DLLs are copied to HumbleEditor
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/GLEW/bin/Release/x64/glew32.dll ../bin/" .. outputdir .. "/HumbleEditor"),

            -- Ensure the GLEW and FMOD DLLs are copied to HumbleApp
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} ../Dependencies/GLEW/bin/Release/x64/glew32.dll ../bin/" .. outputdir .. "/HumbleApp"),
        }

    filter "configurations:Debug"
        defines { "DEBUG" }
        runtime "Debug"
        symbols "On"

        links
        {
            "fmodL_vc.lib",
            "shaderc_sharedd.lib",
            "spirv-cross-cored.lib",
            "spirv-cross-glsld.lib",
        }

        postbuildcommands
        {
            -- Ensure the GLEW and FMOD DLLs are copied to HumbleEditor
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/FMOD/core/lib/x64/fmodL.dll ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} %{cfg.buildtarget.directory}/Humble2.pdb ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} %{cfg.buildtarget.directory}/Humble2.dll ../bin/" .. outputdir .. "/HumbleEditor"),

            -- Ensure the GLEW and FMOD DLLs are copied to HumbleApp
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} ../Dependencies/FMOD/core/lib/x64/fmodL.dll ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} %{cfg.buildtarget.directory}/Humble2.pdb ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} %{cfg.buildtarget.directory}/Humble2.dll ../bin/" .. outputdir .. "/HumbleApp"),
        }
        
    filter "configurations:Release"
        defines { "RELEASE" }
        runtime "Release"
        optimize "On"
        
        links
        {
            "fmod_vc.lib",
            "shaderc_shared.lib",
            "spirv-cross-core.lib",
            "spirv-cross-glsl.lib",
        }

        postbuildcommands
        {
            -- Ensure the GLEW and FMOD DLLs are copied to HumbleEditor
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/FMOD/core/lib/x64/fmod.dll ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} %{cfg.buildtarget.directory}/Humble2.pdb ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} %{cfg.buildtarget.directory}/Humble2.dll ../bin/" .. outputdir .. "/HumbleEditor"),

            -- Ensure the GLEW and FMOD DLLs are copied to HumbleApp
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} ../Dependencies/FMOD/core/lib/x64/fmod.dll ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} %{cfg.buildtarget.directory}/Humble2.pdb ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} %{cfg.buildtarget.directory}/Humble2.dll ../bin/" .. outputdir .. "/HumbleApp"),
        }

    filter "configurations:Dist"
        defines { "DIST" }
        runtime "Release"
        optimize "Full"
        symbols "Off"
        
        links
        {
            "fmod_vc.lib",
            "shaderc_shared.lib",
            "spirv-cross-core.lib",
            "spirv-cross-glsl.lib",
        }

        postbuildcommands
        {
            -- Ensure the GLEW and FMOD DLLs are copied to HumbleEditor
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/FMOD/core/lib/x64/fmod.dll ../bin/" .. outputdir .. "/HumbleEditor"),

            -- Ensure the GLEW and FMOD DLLs are copied to HumbleApp
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} ../Dependencies/FMOD/core/lib/x64/fmod.dll ../bin/" .. outputdir .. "/HumbleApp"),
        }
        
    filter "configurations:Emscripten"
        defines { "EMSCRIPTEN" }
        runtime "Release"
        optimize "On"
        
        links
        {
            "fmod_vc.lib",
            "shaderc_shared.lib",
            "spirv-cross-core.lib",
            "spirv-cross-glsl.lib",
        }

        postbuildcommands
        {
            -- Ensure the GLEW and FMOD DLLs are copied to HumbleEditor
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleEditor"),
            ("{COPY} ../Dependencies/FMOD/core/lib/x64/fmod.dll ../bin/" .. outputdir .. "/HumbleEditor"),

            -- Ensure the GLEW and FMOD DLLs are copied to HumbleApp
            ("{MKDIR} ../bin/" .. outputdir .. "/HumbleApp"),
            ("{COPY} ../Dependencies/FMOD/core/lib/x64/fmod.dll ../bin/" .. outputdir .. "/HumbleApp"),
        }
