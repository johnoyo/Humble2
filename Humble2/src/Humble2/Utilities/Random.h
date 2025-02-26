#pragma once

#include "glm/glm.hpp"

#include <random>
#include <assert.h>
#include <stdint.h>

namespace HBL2
{
	class Random
	{
	public:
		static void Initialize()
		{
			s_RandomEngine.seed(std::random_device()());
		}

		static double Double(double floor, double ceiling)
		{
			assert(floor < ceiling);
			return (double)((((double)s_Distribution(s_RandomEngine) / (double)(std::numeric_limits<uint64_t>::max)()) * (ceiling - floor) + floor));
		}

		static float Float(float floor, float ceiling)
		{
			assert(floor < ceiling);
			return ((float)s_Distribution(s_RandomEngine) / (float)(std::numeric_limits<uint64_t>::max)()) * (ceiling - floor) + floor;
		}

		static int32_t Int32()
		{
			return Int32((std::numeric_limits<int32_t>::min)(), (std::numeric_limits<int32_t>::max)());
		}

		static int32_t Int32(int32_t floor, int32_t ceiling)
		{
			assert(floor < ceiling);
			return (int32_t)((((float)s_Distribution(s_RandomEngine) / (float)(std::numeric_limits<uint64_t>::max)()) * (ceiling - floor) + floor));
		}

		static int64_t Int64()
		{
			return Int64((std::numeric_limits<int64_t>::min)(), (std::numeric_limits<int64_t>::max)());
		}

		static int64_t Int64(int64_t floor, int64_t ceiling)
		{
			assert(floor < ceiling);
			return (int64_t)((((double)s_Distribution(s_RandomEngine) / (double)(std::numeric_limits<uint64_t>::max)()) * (ceiling - floor) + floor));
		}

		static uint32_t UInt32()
		{
			return UInt32(0, (std::numeric_limits<uint32_t>::max)());
		}

		static uint32_t UInt32(uint32_t floor, uint32_t ceiling)
		{
			assert(floor < ceiling);
			return (uint32_t)((((float)s_Distribution(s_RandomEngine) / (float)(std::numeric_limits<uint64_t>::max)()) * (ceiling - floor) + floor));
		}

		static uint64_t UInt64()
		{
			return UInt64(0, (std::numeric_limits<uint64_t>::max)());
		}

		static uint64_t UInt64(uint64_t floor, uint64_t ceiling)
		{
			assert(floor < ceiling);
			return (uint64_t)((((double)s_Distribution(s_RandomEngine) / (double)(std::numeric_limits<uint64_t>::max)()) * (ceiling - floor) + floor));
		}

		static glm::vec2 Vec2(float floor, float ceiling)
		{
			assert(floor < ceiling);
			return glm::vec2(Float(floor, ceiling), Float(floor, ceiling));
		}

		static glm::vec3 Vec3(float floor, float ceiling)
		{
			assert(floor < ceiling);
			return glm::vec3(Float(floor, ceiling), Float(floor, ceiling), Float(floor, ceiling));
		}

		static glm::vec4 Vec4(float floor, float ceiling)
		{
			assert(floor < ceiling);
			return glm::vec4(Float(floor, ceiling), Float(floor, ceiling), Float(floor, ceiling), Float(floor, ceiling));
		}

	private:
		static thread_local std::mt19937_64 s_RandomEngine;
		static std::uniform_int_distribution<std::mt19937_64::result_type> s_Distribution;
	};
}
