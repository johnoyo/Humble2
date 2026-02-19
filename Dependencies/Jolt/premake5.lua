project "Jolt"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    staticruntime "Off"
    systemversion "latest"
    multiprocessorcompile "On"

    targetdir ("jolt/bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("jolt/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        "jolt/Jolt/**.h",
        "jolt/Jolt/**.cpp"
    }

    includedirs
    {
        "jolt/"
    }

    defines (JoltDefines)

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        optimize "Off"
        -- Enable all warnings as errors in Debug
		warnings "Extra"
        fatalwarnings "All"
		
        -- If you want to override MSVC CXX flags exactly like CMake's OVERRIDE_CXX_FLAGS:
        buildoptions { "/GS", "/Od", "/Ob0", "/RTC1" }
		
    filter "configurations:Release"
        runtime "Release"
        symbols "Off"
        optimize "On"
		warnings "Extra"
        fatalwarnings "On"
		
        -- Override MSVC's Release CXX flags per CMake's OVERRIDE_CXX_FLAGS:
        buildoptions { "/GS-", "/Gy", "/O2", "/Oi", "/Ot", "/GL" }
        linkoptions { "/LTCG" }
		
    filter "configurations:Dist"
        runtime "Release"
        symbols "Off"
        optimize "Full"
		warnings "Extra"
        fatalwarnings "On"

        -- Match Release settings (no debug symbols, optimized, LTO)
        buildoptions { "/GS-", "/Gy", "/O2", "/Oi", "/Ot", "/GL" }
        linkoptions { "/LTCG" }