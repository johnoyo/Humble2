-- main solution file.
workspace "HumbleGameEngine2"
    architecture "x64"

    configurations 
    { 
        "Debug",
        "Release",
        "Emscripten"
    }

    startproject "SampleApp"

-- Variable to hold output directory.
outputdir = "%{cfg.buildcfg}-%{cfg.architecture}"

include "Humble2"
include "SampleApp"