#pragma once

#include <random>
#include <assert.h>
#include <stdint.h>

namespace HBL2 
{
	class Random
	{
	public:
		static void Init()
		{
			s_RandomEngine.seed(std::random_device()());
		}

		static double Double(double floor, double ceiling)
		{
			assert(floor < ceiling);
			return (double)((((double)s_Distribution(s_RandomEngine) / (double)std::numeric_limits<uint64_t>::max()) * (ceiling - floor) + floor));
		}

		static float Float(float floor, float ceiling)
		{
			assert(floor < ceiling);
			return ((float)s_Distribution(s_RandomEngine) / (float)std::numeric_limits<uint64_t>::max()) * (ceiling - floor) + floor;
		}

		static int32_t Int32(int32_t floor, int32_t ceiling)
		{
			assert(floor < ceiling);
			return (int32_t)((((float)s_Distribution(s_RandomEngine) / (float)std::numeric_limits<uint64_t>::max()) * (ceiling - floor) + floor));
		}

		static int64_t Int64(int64_t floor, int64_t ceiling)
		{
			assert(floor < ceiling);
			return (int64_t)((((double)s_Distribution(s_RandomEngine) / (double)std::numeric_limits<uint64_t>::max()) * (ceiling - floor) + floor));
		}

		static uint32_t UInt32(uint32_t floor, uint32_t ceiling)
		{
			assert(floor < ceiling);
			return (uint32_t)((((float)s_Distribution(s_RandomEngine) / (float)std::numeric_limits<uint64_t>::max()) * (ceiling - floor) + floor));
		}

		static uint64_t UInt64(uint64_t floor, uint64_t ceiling)
		{
			assert(floor < ceiling);
			return (uint64_t)((((double)s_Distribution(s_RandomEngine) / (double)std::numeric_limits<uint64_t>::max()) * (ceiling - floor) + floor));
		}

	private:
		static std::mt19937_64 s_RandomEngine;
		static std::uniform_int_distribution<std::mt19937_64::result_type> s_Distribution;
	};

}