#pragma once

#include "glm/gtc/noise.hpp"

namespace HBL2::Math
{
	/*
	 * @brief Computes 2D Perlin noise in [0,1] range.
	 */
	static float PerlinNoise(float x, float y)
	{
		float noise = glm::perlin(glm::vec2{ x, y });	// [-1, 1]
		return noise * 0.5f + 0.5f;						// [0, 1]
	}
}