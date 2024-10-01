#pragma once

#include "Humble2/Utilities/Log.h"
#include "Humble2/Utilities/ProfilerScope.h"

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

namespace HBL
{
	struct Buffer
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 TextureCoord;
		float TextureID;
		glm::vec3 Normal;
	};
}

struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 UV;
	//glm::vec4 Color;
	//glm::vec3 Tangent;
};

using UUID = uint64_t;
