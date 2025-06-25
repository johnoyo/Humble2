project "Jolt"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"                   -- Jolt requires C++17 :contentReference[oaicite:0]{index=0}
    staticruntime "Off"                  -- Use the DLL runtime by default
    systemversion "latest"               -- Target the latest Windows SDK (VS2022)

    flags { "MultiProcessorCompile" }

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
        symbols "On"                                -- Generate /Zi and link /DEBUG (CMake's GENERATE_DEBUG_SYMBOLS ON) :contentReference[oaicite:10]{index=10}
        optimize "Off"
        -- Enable all warnings as errors in Debug
		warnings "Extra"
        flags { "FatalWarnings" }   -- /Wall /WX in MSVC
		
        -- If you want to override MSVC CXX flags exactly like CMake's OVERRIDE_CXX_FLAGS:
        buildoptions { "/GS", "/Od", "/Ob0", "/RTC1" }
		
		filter "configurations:Release"
        runtime "Release"
        symbols "Off"
        optimize "On"
		warnings "Extra"
        flags { "FatalWarnings" }   -- /Wall /WX in MSVC
		
        -- Override MSVC's Release CXX flags per CMake's OVERRIDE_CXX_FLAGS:
        buildoptions { "/GS-", "/Gy", "/O2", "/Oi", "/Ot", "/GL" }
        linkoptions { "/LTCG" }
		
		filter "configurations:Dist"
        runtime "Release"
        symbols "Off"
        optimize "On"
		warnings "Extra"
        flags { "FatalWarnings" }   -- Keep warnings as errors

        -- Match Release settings (no debug symbols, optimized, LTO)
        buildoptions { "/GS-", "/Gy", "/O2", "/Oi", "/Ot", "/GL" }
        linkoptions { "/LTCG" }