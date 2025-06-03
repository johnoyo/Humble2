#pragma once

namespace HBL2
{
	namespace Physics
	{
		enum class BodyType
		{
			Static = 0,
			Kinematic,
			Dynamic,
		};

		using ID = uint64_t;

		static constexpr uint64_t InvalidID = UINT64_MAX;
	}
}