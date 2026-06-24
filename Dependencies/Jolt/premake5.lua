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

    externalincludedirs
    {
        "jolt/"
    }

    filter "system:windows"
		systemversion "latest"
        defines { table.unpack(JoltDefines) }

        filter { "system:windows", "configurations:Debug" }
            runtime "Debug"
            symbols "On"
            optimize "Off"
            -- Enable all warnings as errors in Debug
            warnings "Extra"
            fatalwarnings "All"
            
            -- If you want to override MSVC CXX flags exactly like CMake's OVERRIDE_CXX_FLAGS:
            buildoptions { "/GS", "/Od", "/Ob0", "/RTC1" }
            
        filter { "system:windows", "configurations:Release" }
            runtime "Release"
            symbols "Off"
            optimize "On"
            warnings "Extra"
            fatalwarnings "All"
            
            -- Override MSVC's Release CXX flags per CMake's OVERRIDE_CXX_FLAGS:
            buildoptions { "/GS-", "/Gy", "/O2", "/Oi", "/Ot", "/GL" }
            linkoptions { "/LTCG" }
            
        filter { "system:windows", "configurations:Dist" }
            runtime "Release"
            symbols "Off"
            optimize "Full"
            warnings "Extra"
            fatalwarnings "All"

            -- Match Release settings (no debug symbols, optimized, LTO)
            buildoptions { "/GS-", "/Gy", "/O2", "/Oi", "/Ot", "/GL" }
            linkoptions { "/LTCG" }
    
	filter "system:macosx"
		systemversion "latest"
        defines { table.unpack(JoltDefinesArm) }

        filter { "system:macosx", "configurations:Debug" }
            runtime "Debug"
            symbols "On"
            optimize "Off"
            -- Enable all warnings as errors in Debug
            warnings "Extra"
            fatalwarnings "All"
            
        filter { "system:macosx", "configurations:Release" }
            runtime "Release"
            symbols "Off"
            optimize "On"
            warnings "Extra"
            fatalwarnings "All"
            
            -- Override MSVC's Release CXX flags per CMake's OVERRIDE_CXX_FLAGS:
            buildoptions { "/GS-", "/Gy", "/O2", "/Oi", "/Ot", "/GL" }
            linkoptions { "/LTCG" }
            
        filter { "system:macosx", "configurations:Dist" }
            runtime "Release"
            symbols "Off"
            optimize "Full"
            warnings "Extra"
            fatalwarnings "All"

            -- Match Release settings (no debug symbols, optimized, LTO)
            buildoptions { "/GS-", "/Gy", "/O2", "/Oi", "/Ot", "/GL" }
            linkoptions { "/LTCG" }