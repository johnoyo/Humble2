#include "Random.h"

namespace HBL
{
	std::mt19937_64 Random::s_RandomEngine;
	std::uniform_int_distribution<std::mt19937_64::result_type> Random::s_Distribution;
}
