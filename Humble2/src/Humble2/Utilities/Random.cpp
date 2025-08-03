#include "Random.h"

namespace
{
	thread_local std::mt19937_64 g_RandomEngine{ std::random_device{}() };
	std::uniform_int_distribution<std::mt19937_64::result_type> g_Distribution;
}

namespace HBL2
{
	std::mt19937_64& Random::RandomEngine()
	{
		return g_RandomEngine;
	}

	void Random::Initialize()
	{
		g_RandomEngine.seed(std::random_device()());
	}

	void Random::Seed(uint64_t seed)
	{
		g_RandomEngine.seed(seed);
	}

	double Random::Double(double floor, double ceiling)
	{
		assert(floor < ceiling);
		return (double)((((double)g_Distribution(g_RandomEngine) / (double)(std::numeric_limits<uint64_t>::max)()) * (ceiling - floor) + floor));
	}

	float Random::Float(float floor, float ceiling)
	{
		assert(floor < ceiling);
		return ((float)g_Distribution(g_RandomEngine) / (float)(std::numeric_limits<uint64_t>::max)()) * (ceiling - floor) + floor;
	}

	int32_t Random::Int32()
	{
		return Int32((std::numeric_limits<int32_t>::min)(), (std::numeric_limits<int32_t>::max)());
	}

	int32_t Random::Int32(int32_t floor, int32_t ceiling)
	{
		assert(floor < ceiling);
		return (int32_t)((((float)g_Distribution(g_RandomEngine) / (float)(std::numeric_limits<uint64_t>::max)()) * (ceiling - floor) + floor));
	}

	int64_t Random::Int64()
	{
		return Int64((std::numeric_limits<int64_t>::min)(), (std::numeric_limits<int64_t>::max)());
	}

	int64_t Random::Int64(int64_t floor, int64_t ceiling)
	{
		assert(floor < ceiling);
		return (int64_t)((((double)g_Distribution(g_RandomEngine) / (double)(std::numeric_limits<uint64_t>::max)()) * (ceiling - floor) + floor));
	}

	uint32_t Random::UInt32()
	{
		return UInt32(0, (std::numeric_limits<uint32_t>::max)());
	}

	uint32_t Random::UInt32(uint32_t floor, uint32_t ceiling)
	{
		assert(floor < ceiling);
		return (uint32_t)((((float)g_Distribution(g_RandomEngine) / (float)(std::numeric_limits<uint64_t>::max)()) * (ceiling - floor) + floor));
	}

	uint64_t Random::UInt64()
	{
		return UInt64(0, (std::numeric_limits<uint64_t>::max)());
	}

	uint64_t Random::UInt64(uint64_t floor, uint64_t ceiling)
	{
		assert(floor < ceiling);
		return (uint64_t)((((double)g_Distribution(g_RandomEngine) / (double)(std::numeric_limits<uint64_t>::max)()) * (ceiling - floor) + floor));
	}

	glm::vec2 Random::Vec2(float floor, float ceiling)
	{
		assert(floor < ceiling);
		return glm::vec2(Float(floor, ceiling), Float(floor, ceiling));
	}

	glm::vec3 Random::Vec3(float floor, float ceiling)
	{
		assert(floor < ceiling);
		return glm::vec3(Float(floor, ceiling), Float(floor, ceiling), Float(floor, ceiling));
	}

	glm::vec4 Random::Vec4(float floor, float ceiling)
	{
		assert(floor < ceiling);
		return glm::vec4(Float(floor, ceiling), Float(floor, ceiling), Float(floor, ceiling), Float(floor, ceiling));
	}
}