local KTX_ROOT = "ktx"
local LIB_ROOT = KTX_ROOT .. "/lib"
local BASIS_ROOT = KTX_ROOT .. "/external/basis_universal"
local DFD_ROOT = KTX_ROOT .. "/external/dfdutils"
local ASTC_ROOT = KTX_ROOT .. "/external/astc-encoder"

project "KTX-Software"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"
    cdialect "C11"
    staticruntime "Off"
	multiprocessorcompile "On"

    targetdir (KTX_ROOT .. "/bin/" .. outputdir .. "/%{prj.name}")
    objdir (KTX_ROOT .. "/bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        -- Public KTX headers.
        LIB_ROOT .. "/include/ktx.h",
        LIB_ROOT .. "/include/ktxvulkan.h",
        KTX_ROOT .. "/external/dfdutils/KHR/khr_df.h",

        -- libktx common runtime.
        LIB_ROOT .. "/src/astc_codec.cpp",
        LIB_ROOT .. "/src/basis_sgd.h",
        LIB_ROOT .. "/src/basis_transcode.cpp",
        LIB_ROOT .. "/src/miniz_wrapper.cpp",
        LIB_ROOT .. "/src/checkheader.c",
        LIB_ROOT .. "/src/etcunpack.cxx",
        LIB_ROOT .. "/src/filestream.c",
        LIB_ROOT .. "/src/filestream.h",
        LIB_ROOT .. "/src/formatsize.h",
        LIB_ROOT .. "/src/gl_format.h",
        LIB_ROOT .. "/src/glformat_str.c",
        LIB_ROOT .. "/src/hashlist.c",
        LIB_ROOT .. "/src/info.c",
        LIB_ROOT .. "/src/ktxint.h",
        LIB_ROOT .. "/src/memstream.c",
        LIB_ROOT .. "/src/memstream.h",
        LIB_ROOT .. "/src/strings.c",
        LIB_ROOT .. "/src/swap.c",
        LIB_ROOT .. "/src/texture.c",
        LIB_ROOT .. "/src/texture.h",
        LIB_ROOT .. "/src/texture1.c",
        LIB_ROOT .. "/src/texture1.h",
        LIB_ROOT .. "/src/texture2.c",
        LIB_ROOT .. "/src/texture2.h",
        LIB_ROOT .. "/src/texture_funcs.inl",
        LIB_ROOT .. "/src/uthash.h",
        LIB_ROOT .. "/src/vk2gl.h",
        LIB_ROOT .. "/src/vk_format.h",
        LIB_ROOT .. "/src/vkFormat2glFormat.inl",
        LIB_ROOT .. "/src/vkFormat2glInternalFormat.inl",
        LIB_ROOT .. "/src/vkFormat2glType.inl",
        LIB_ROOT .. "/src/vkformat_check.c",
        LIB_ROOT .. "/src/vkformat_check_variant.c",
        LIB_ROOT .. "/src/vkformat_enum.h",
        LIB_ROOT .. "/src/vkformat_str.c",
        LIB_ROOT .. "/src/vkformat_typesize.c",

        -- Full/write-capable runtime API.
        LIB_ROOT .. "/src/basis_encode.cpp",
        LIB_ROOT .. "/src/writer1.c",
        LIB_ROOT .. "/src/writer2.c",

        -- OpenGL upload API.
        LIB_ROOT .. "/src/gl_funclist.inl",
        LIB_ROOT .. "/src/gl_funcs.c",
        LIB_ROOT .. "/src/gl_funcs.h",
        LIB_ROOT .. "/src/glloader.c",

        -- Vulkan upload API. No Vulkan loader library is required; KTX uses
        -- function pointers supplied by the application/runtime loader.
        LIB_ROOT .. "/src/vk_funcs.c",
        LIB_ROOT .. "/src/vk_funcs.h",
        LIB_ROOT .. "/src/vkloader.c",

        -- ETC software decoder.
        KTX_ROOT .. "/external/etcdec/etcdec.cxx",

        -- DFD utilities, embedded into this archive.
        DFD_ROOT .. "/createdfd.c",
        DFD_ROOT .. "/colourspaces.c",
        DFD_ROOT .. "/interpretdfd.c",
        DFD_ROOT .. "/printdfd.c",
        DFD_ROOT .. "/queries.c",
        DFD_ROOT .. "/vk2dfd.c",
        DFD_ROOT .. "/**.h",
        DFD_ROOT .. "/**.inl",

        -- Basis Universal encoder + transcoder + bundled Zstd.
        BASIS_ROOT .. "/encoder/basisu_backend.cpp",
        BASIS_ROOT .. "/encoder/basisu_basis_file.cpp",
        BASIS_ROOT .. "/encoder/basisu_comp.cpp",
        BASIS_ROOT .. "/encoder/basisu_enc.cpp",
        BASIS_ROOT .. "/encoder/basisu_etc.cpp",
        BASIS_ROOT .. "/encoder/basisu_frontend.cpp",
        BASIS_ROOT .. "/encoder/basisu_gpu_texture.cpp",
        BASIS_ROOT .. "/encoder/basisu_pvrtc1_4.cpp",
        BASIS_ROOT .. "/encoder/basisu_resampler.cpp",
        BASIS_ROOT .. "/encoder/basisu_resample_filters.cpp",
        BASIS_ROOT .. "/encoder/basisu_ssim.cpp",
        BASIS_ROOT .. "/encoder/basisu_uastc_enc.cpp",
        BASIS_ROOT .. "/encoder/basisu_bc7enc.cpp",
        BASIS_ROOT .. "/encoder/jpgd.cpp",
        BASIS_ROOT .. "/encoder/basisu_kernels_sse.cpp",
        BASIS_ROOT .. "/encoder/basisu_opencl.cpp",
        BASIS_ROOT .. "/encoder/pvpngreader.cpp",
        BASIS_ROOT .. "/encoder/basisu_uastc_hdr_4x4_enc.cpp",
        BASIS_ROOT .. "/encoder/basisu_astc_hdr_6x6_enc.cpp",
        BASIS_ROOT .. "/encoder/basisu_astc_hdr_common.cpp",
        BASIS_ROOT .. "/encoder/basisu_astc_ldr_common.cpp",
        BASIS_ROOT .. "/encoder/basisu_astc_ldr_encode.cpp",
        BASIS_ROOT .. "/encoder/3rdparty/android_astc_decomp.cpp",
        BASIS_ROOT .. "/encoder/3rdparty/tinyexr.cpp",
        BASIS_ROOT .. "/transcoder/basisu_transcoder.cpp",
        BASIS_ROOT .. "/zstd/zstd.c",
        BASIS_ROOT .. "/encoder/**.h",
        BASIS_ROOT .. "/transcoder/**.h",
        BASIS_ROOT .. "/zstd/**.h",

        -- ASTC encoder library. CLI translation units are removed below.
        ASTC_ROOT .. "/Source/**.cpp",
        ASTC_ROOT .. "/Source/**.h",
        ASTC_ROOT .. "/Source/**.inl",
        ASTC_ROOT .. "/include/**.h",
    }

    removefiles
    {
        -- ASTC command-line frontend and tests; keep astcenc_entry.cpp because
        -- it implements the public astcenc library API.
        ASTC_ROOT .. "/Source/astcenccli_*.cpp",
        ASTC_ROOT .. "/Source/astcenccli_*.h",
        ASTC_ROOT .. "/Source/UnitTest/**",
        ASTC_ROOT .. "/Test/**",

        -- Never compile BasisU tools/examples into the library.
        BASIS_ROOT .. "/basisu_tool.cpp",
        BASIS_ROOT .. "/example/**",
        BASIS_ROOT .. "/example_capi/**",
        BASIS_ROOT .. "/example_transcoding/**",
    }

    externalincludedirs
    {
        LIB_ROOT .. "/include",
        LIB_ROOT .. "/src",
        KTX_ROOT .. "/external",
        KTX_ROOT .. "/external/dfdutils",
        KTX_ROOT .. "/external/dfdutils/KHR",
        KTX_ROOT .. "/utils",
        KTX_ROOT .. "/other_include",

        DFD_ROOT,
        DFD_ROOT .. "/vulkan",

        BASIS_ROOT,
        BASIS_ROOT .. "/encoder",
        BASIS_ROOT .. "/transcoder",
        BASIS_ROOT .. "/zstd",

        ASTC_ROOT .. "/include",
        ASTC_ROOT .. "/Source",
    }

    defines
    {
        "LIBKTX",
        "KHRONOS_STATIC",
        "KTX_FEATURE_KTX1=1",
        "KTX_FEATURE_KTX2=1",
        "KTX_FEATURE_WRITE=1",
        "SUPPORT_SOFTWARE_ETC_UNPACK=1",

        -- Match the KTX CMake defaults: bundled Zstd, no OpenCL.
        "BASISD_SUPPORT_KTX2_ZSTD=1",
        "BASISU_SUPPORT_OPENCL=0",
        "BASISU_WASI_THREADS=0",
        "BASISD_SUPPORT_FXT1=0",

        -- Required when BasisU is embedded in a static Windows library.
        "BASISU_NO_ITERATOR_DEBUG_LEVEL",
    }
	
    filter "system:windows"
		systemversion "latest"
		buildoptions { "-msse4.1", "/utf-8" }
		defines
		{
			"BASISU_SUPPORT_SSE=1",
            "ASTCENC_SSE=41",
			"_CRT_SECURE_NO_WARNINGS",
			"_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR",
		}
	
    filter "system:macosx"
		systemversion "latest"
		links { "pthread", "m" }

		defines
		{
			"BASISU_SUPPORT_SSE=0",
			"ASTCENC_NEON=1",
		}
	
    filter "system:linux"
        systemversion "latest"
		buildoptions { "-fPIC" }
        links { "pthread", "dl", "m" }

		defines
		{
			"BASISU_SUPPORT_SSE=1",
            "ASTCENC_SSE=41",
		}

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        defines { "_DEBUG", "DEBUG" }

    filter "configurations:Release"
        runtime "Release"
        optimize "On"

    filter "configurations:Dist"
        runtime "Release"
        optimize "Full"
        symbols "Off"

    filter "configurations:Emscripten"
        defines
        {
            "EMSCRIPTEN",
            "BASISU_SUPPORT_SSE=0",
            "BASISD_SUPPORT_ATC=0",
            "BASISD_SUPPORT_PVRTC2=0",
            "BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY=0",
            "KTX_OMIT_VULKAN=1",
        }
        runtime "Release"
        optimize "On"
        buildoptions
        {
            "-Wno-nested-anon-types",
            "-Wno-gnu-anonymous-struct",
        }

    filter {}
