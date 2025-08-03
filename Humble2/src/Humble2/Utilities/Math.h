#pragma once

#include <glm/glm.hpp>
#include "glm/gtc/noise.hpp"

namespace HBL2::Math
{
	static inline float Lerp(float a, float b, float value)
	{
		return glm::mix(a, b, value);
	}

	static inline glm::vec2 Lerp(const glm::vec2& a, const glm::vec2& b, const glm::vec2& value)
	{
		return glm::mix(a, b, value);
	}

	static inline glm::vec3 Lerp(const glm::vec3& a, const glm::vec3& b, const glm::vec3& value)
	{
		return glm::mix(a, b, value);
	}

	static inline float InverseLerp(float a, float b, float value)
	{
		if (a == b) return 0.0f;
		return glm::clamp((value - a) / (b - a), 0.0f, 1.0f);
	}

	static inline glm::vec2 InverseLerp(const glm::vec2& a, const glm::vec2& b, const glm::vec2& value)
	{
		return glm::clamp((value - a) / (b - a), glm::vec2(0.0f), glm::vec2(1.0f));
	}

	static inline glm::vec3 InverseLerp(const glm::vec3& a, const glm::vec3& b, const glm::vec3& value)
	{
		return glm::clamp((value - a) / (b - a), glm::vec3(0.0f), glm::vec3(1.0f));
	}

	/*
	 * @brief Computes 2D Perlin noise in [0,1] range.
	 */
	static inline float PerlinNoise(float x, float y)
	{
		float noise = glm::perlin(glm::vec2{ x, y });	// [-1, 1]
		return noise * 0.5f + 0.5f;						// [0, 1]
	}
}