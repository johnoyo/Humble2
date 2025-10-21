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

JoltDefines = {
    -- Enable/assert macros
    "JPH_ENABLE_ASSERTS",                        -- (CMake: USE_ASSERTS OFF by default; we enable it here) :contentReference[oaicite:1]{index=1}

    -- Object Layer Bits (16 or 32). Default in CMake is 16. :contentReference[oaicite:2]{index=2}
    "JPH_OBJECT_LAYER_BITS=16",

    -- Floating-point exception support (Windows only). CMake's FLOATING_POINT_EXCEPTIONS_ENABLED is ON by default. :contentReference[oaicite:3]{index=3}
    "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED",

    -- Cross-platform determinism (CMake default OFF)
    "JPH_CROSS_PLATFORM_DETERMINISTIC=0",         -- If you need determinism across platforms, set to 1. :contentReference[oaicite:4]{index=4}

    -- CPU instruction sets (ON by default in CMake for x86/x64) :contentReference[oaicite:5]{index=5}
    "JPH_USE_SSE4_1",
    "JPH_USE_SSE4_2",
    "JPH_USE_AVX",
    "JPH_USE_AVX2",
    "JPH_USE_LZCNT",
    "JPH_USE_TZCNT",
    "JPH_USE_F16C",
    "JPH_USE_FMADD",
    -- "JPH_USE_AVX512",                          -- Default OFF in CMake; uncomment to enable

    -- Enable profiling & debug renderer in Debug/Release if desired :contentReference[oaicite:6]{index=6}
    "JPH_PROFILER_IN_DEBUG_AND_RELEASE",
    "JPH_DEBUG_RENDERER",

    -- Enable ObjectStream support (CMake's ENABLE_OBJECT_STREAM ON by default) :contentReference[oaicite:7]{index=7}
    "JPH_OBJECT_STREAM",

    -- Disable C++ exceptions & RTTI (Jolt's CMake default: OFF) :contentReference[oaicite:8]{index=8}
    "JPH_CPP_EXCEPTIONS_ENABLED=0",
    "JPH_CPP_RTTI_ENABLED=0",
}

group "Dependencies"
    include "Dependencies/ImGui"
    include "Dependencies/YAML-Cpp"
    include "Dependencies/Box2D"
    include "Dependencies/Jolt"
group ""

group "Core"
    include "Humble2"
group ""

include "HumbleEditor"
include "HumbleApp"