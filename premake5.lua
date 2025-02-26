-- main solution file.

VULKAN_SDK = os.getenv("VULKAN_SDK")

workspace "HumbleGameEngine2"
    architecture "x64"

    configurations 
    { 
        "Debug",
        "Release",
        "Dist",
        "Emscripten"
    }

    startproject "HumbleEditor"

-- Variable to hold output directory.
outputdir = "%{cfg.buildcfg}-%{cfg.architecture}"

group "Dependencies"
    include "Dependencies/ImGui"
    include "Dependencies/YAML-Cpp"
group ""

group "Core"
    include "Humble2"
group ""

include "HumbleEditor"
include "HumbleApp"