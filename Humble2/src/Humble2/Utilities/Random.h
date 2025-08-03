#pragma once

#include "Humble2API.h"

#include "glm/glm.hpp"

#include <random>
#include <assert.h>
#include <stdint.h>

namespace HBL2
{
	class HBL2_API Random
	{
	public:
		static void Initialize();

		static void Seed(uint64_t seed = std::random_device()());

		static double Double(double floor, double ceiling);

		static float Float(float floor, float ceiling);

		static int32_t Int32();

		static int32_t Int32(int32_t floor, int32_t ceiling);

		static int64_t Int64();

		static int64_t Int64(int64_t floor, int64_t ceiling);

		static uint32_t UInt32();

		static uint32_t UInt32(uint32_t floor, uint32_t ceiling);

		static uint64_t UInt64();

		static uint64_t UInt64(uint64_t floor, uint64_t ceiling);

		static glm::vec2 Vec2(float floor, float ceiling);

		static glm::vec3 Vec3(float floor, float ceiling);

		static glm::vec4 Vec4(float floor, float ceiling);

	private:
		static std::mt19937_64& RandomEngine();
	};
}
