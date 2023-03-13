#pragma once

#include "Humble2/Utilities/Log.h"
#include "Humble2/Utilities/JobSystem.h"
#include "Humble2/Utilities/ProfilerScope.h"

#ifdef DEBUG
	#define HBL_PROFILE(...) HBL::ProfilerScope profiler = HBL::ProfilerScope(__VA_ARGS__);
	#define HBL_FUNC_PROFILE() HBL_PROFILE(__FUNCTION__)
#else
	#define HBL_PROFILE(...)
	#define HBL_FUNC_PROFILE()
#endif

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image/stb_image.h>

struct Buffer
{
	glm::vec2 Position;
	glm::vec4 Color;
	glm::vec2 TextureCoord;
	float TextureID;
};