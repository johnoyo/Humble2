#pragma once

#include "Humble2Core.h"

// Just a POD struct
HBL2_COMPONENT(Speedster,
{
	bool Enabled = true;
    float Force = 5.f;

	float Score = 0.f;
	float Power = 100.f;
	float Speed = 7.f;
})

// Register members
REGISTER_HBL2_COMPONENT(Speedster,
	HBL2_COMPONENT_MEMBER(Speedster, Enabled)
	HBL2_COMPONENT_MEMBER(Speedster, Force)
)
