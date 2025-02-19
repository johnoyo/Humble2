#pragma once

#include "Humble2/Utilities/Log.h"
#include "Humble2/Utilities/ProfilerScope.h"

#include "Humble2API.h"

#ifdef DEBUG
	#define HBL2_PROFILE(...) HBL2::ProfilerScope profiler = HBL2::ProfilerScope(__VA_ARGS__);
	#define HBL2_FUNC_PROFILE() HBL2_PROFILE(__FUNCTION__)

	#ifdef HBL2_PLATFORM_WINDOWS
		#define HBL2_DEBUGBREAK() __debugbreak()
	#elif HBL2_PLATFORM_LINUX
		#include <signal.h>
		#define HBL_DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform is not supported yet!"
	#endif

	#define HBL2_ENABLE_ASSERTS // TODO: figure out whats wrong and it needs asserts enabled.
#else
	#define HBL2_PROFILE(...)
	#define HBL2_FUNC_PROFILE()
	#define HBL2_DEBUGBREAK()
	#define HBL2_ENABLE_ASSERTS
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
	constexpr size_t operator""_KB(unsigned long long value) { return value * 1024; }
	constexpr size_t operator""_MB(unsigned long long value) { return value * 1024 * 1024; }
	constexpr size_t operator""_GB(unsigned long long value) { return value * 1024 * 1024 * 1024; }
}
