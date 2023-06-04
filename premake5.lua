-- main solution file.
workspace "HumbleGameEngine2"
    architecture "x64"

    configurations 
    { 
        "Debug",
        "Release",
        "Emscripten"
    }

    startproject "HumbleApp"

-- Variable to hold output directory.
outputdir = "%{cfg.buildcfg}-%{cfg.architecture}"

group "Dependencies"
    include "Dependencies/ImGui"
group ""

group "Core"
    include "Humble2"
group ""

include "HumbleEditor"
include "HumbleApp"