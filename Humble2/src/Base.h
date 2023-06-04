#pragma once

#include "Humble2/Utilities/Log.h"
#include "Humble2/Utilities/JobSystem.h"
#include "Humble2/Utilities/ProfilerScope.h"

#ifdef DEBUG
	#define HBL2_PROFILE(...) HBL2::ProfilerScope profiler = HBL2::ProfilerScope(__VA_ARGS__);
	#define HBL2_FUNC_PROFILE() HBL2_PROFILE(__FUNCTION__)
#else
	#define HBL2_PROFILE(...)
	#define HBL2_FUNC_PROFILE()
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