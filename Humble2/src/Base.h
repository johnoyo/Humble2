#pragma once

#include "Humble2/Utilities/Log.h"
#include "Humble2/Utilities/ProfilerScope.h"

#include "Core/Timer.h"

#include "Humble2API.h"

// Detect CPU architecture
#if defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM) || defined(_M_ARM64EC)
	// ARM CPU architecture
	#define HBL2_CPU_ARM
	#if defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
		#define HBL2_CPU_ARCH_BITS 64
		#define HBL2_USE_NEON
	#else
		#error Unsupported 32bit CPU architecture
	#endif
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
	// X86 CPU architecture
	#define HBL2_CPU_X86
	#if defined(__x86_64__) || defined(_M_X64)
		#define HBL2_CPU_ARCH_BITS 64
	#else
		#error Unsupported 32bit CPU architecture
	#endif
	#define HBL2_USE_SSE

	// Detect enabled instruction sets
	#if defined(__AVX512F__) && defined(__AVX512VL__) && defined(__AVX512DQ__) && !defined(HBL2_USE_AVX512)
		#define HBL2_USE_AVX512
	#endif
	#if (defined(__AVX2__) || defined(HBL2_USE_AVX512)) && !defined(HBL2_USE_AVX2)
		#define HBL2_USE_AVX2
	#endif
	#if (defined(__AVX__) || defined(HBL2_USE_AVX2)) && !defined(HBL2_USE_AVX)
		#define HBL2_USE_AVX
	#endif
	#if (defined(__SSE4_2__) || defined(HBL2_USE_AVX)) && !defined(HBL2_USE_SSE4_2)
		#define HBL2_USE_SSE4_2
	#endif
	#if (defined(__SSE4_1__) || defined(HBL2_USE_SSE4_2)) && !defined(HBL2_USE_SSE4_1)
		#define HBL2_USE_SSE4_1
	#endif
	#if (defined(__F16C__) || defined(HBL2_USE_AVX2)) && !defined(HBL2_USE_F16C)
		#define HBL2_USE_F16C
	#endif
	#if (defined(__LZCNT__) || defined(HBL2_USE_AVX2)) && !defined(HBL2_USE_LZCNT)
		#define HBL2_USE_LZCNT
	#endif
	#if (defined(__BMI__) || defined(HBL2_USE_AVX2)) && !defined(HBL2_USE_TZCNT)
		#define HBL2_USE_TZCNT
	#endif
#elif defined(HBL2_PLATFORM_WASM)
	// WebAssembly CPU architecture
	#define HBL2_CPU_WASM
	#if defined(__wasm64__)
		#define HBL2_CPU_ARCH_BITS 64
	#else
		#error Unsupported 32bit CPU architecture
	#endif
	#ifdef __wasm_simd128__
		#define HBL2_USE_SSE
		#define HBL2_USE_SSE4_1
		#define HBL2_USE_SSE4_2
	#endif
#else
	#error Unsupported CPU architecture
#endif

// Define compiler
#if defined(__clang__)
	#define HBL2_COMPILER_CLANG
#elif defined(__GNUC__)
	#define HBL2_COMPILER_GCC
#elif defined(_MSC_VER)
	#define HBL2_COMPILER_MSVC
#endif

#ifdef DEBUG
	#define HBL2_PROFILE(...) HBL2::ProfilerScope profiler = HBL2::ProfilerScope(__VA_ARGS__);
	#define HBL2_FUNC_PROFILE() HBL2_PROFILE(__FUNCTION__)

	#ifdef HBL2_PLATFORM_WINDOWS
		#define HBL2_DEBUGBREAK() __debugbreak()
	#elif HBL2_PLATFORM_LINUX
		#include <signal.h>
		#define HBL2_DEBUGBREAK() raise(SIGTRAP)
    #elif HBL2_PLATFORM_MACOS
		#define HBL2_DEBUGBREAK() __builtin_debugtrap()
	#else
		#error "Platform is not supported yet!"
	#endif

	#define ARENA_DEBUG
	#define HBL2_ENABLE_ASSERTS
#elif RELEASE
	#define HBL2_PROFILE(...) HBL2::ProfilerScope profiler = HBL2::ProfilerScope(__VA_ARGS__);
	#define HBL2_FUNC_PROFILE() HBL2_PROFILE(__FUNCTION__)

	#ifdef HBL2_PLATFORM_WINDOWS
		#define HBL2_DEBUGBREAK() __debugbreak()
	#elif HBL2_PLATFORM_LINUX
		#include <signal.h>
		#define HBL2_DEBUGBREAK() raise(SIGTRAP)
    #elif HBL2_PLATFORM_MACOS
		#define HBL2_DEBUGBREAK() __builtin_debugtrap()
    #else
		#error "Platform is not supported yet!"
	#endif

	#define ARENA_DEBUG
	#define HBL2_ENABLE_ASSERTS
#else
	#define HBL2_PROFILE(...)
	#define HBL2_FUNC_PROFILE()
	#define HBL2_DEBUGBREAK()
#endif

#define HBL2_EXPAND_MACRO(x) x
#define HBL2_STRINGFY_MACRO(x) #x

#ifdef HBL2_ENABLE_ASSERTS
	#define HBL2_ASSERT(x, ...) { if (!(x)) { HBL2_ERROR("Assertion Failed: {0}", __VA_ARGS__); HBL2_DEBUGBREAK(); } }
	#define HBL2_CORE_ASSERT(x, ...) { if (!(x)) { HBL2_CORE_ERROR("Assertion Failed: {0}", __VA_ARGS__); HBL2_DEBUGBREAK(); } }
#else
	#define HBL2_ASSERT(x, ...)
	#define HBL2_CORE_ASSERT(x, ...)
#endif

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stb_image/stb_image.h>

namespace HBL2
{
	using UUID = uint64_t;

	constexpr size_t operator""_B (unsigned long long value) { return value; }
	constexpr size_t B (unsigned long long value) { return value; }
	constexpr size_t operator""_KB(unsigned long long value) { return value * 1024; }
	constexpr size_t KB(unsigned long long value) { return value * 1024; }
	constexpr size_t operator""_MB(unsigned long long value) { return value * 1024 * 1024; }
	constexpr size_t MB(unsigned long long value) { return value * 1024 * 1024; }
	constexpr size_t operator""_GB(unsigned long long value) { return value * 1024 * 1024 * 1024; }
	constexpr size_t GB(unsigned long long value) { return value * 1024 * 1024 * 1024; }
}
