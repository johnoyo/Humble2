project "Box2D"
	kind "StaticLib"
	language "C"
	cdialect "C11"
	staticruntime "Off"

	targetdir ("box2d/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("box2d/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"box2d/src/**.h",
		"box2d/src/**.c",
		"box2d/src/**.cpp",
		"box2d/include/**.h"
	}

	includedirs
	{
		"box2d/include",
		"box2d/src"
	}

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		runtime "Release"
		optimize "On"

	filter "configurations:Dist"
		runtime "Release"
		optimize "On"
