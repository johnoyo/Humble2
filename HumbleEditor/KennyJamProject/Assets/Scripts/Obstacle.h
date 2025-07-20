#pragma once

#include "Humble2Core.h"

// Just a POD struct
HBL2_COMPONENT(Obstacle,
{
    int Value = 1;
})

// Register members
REGISTER_HBL2_COMPONENT(Obstacle,
	HBL2_COMPONENT_MEMBER(Obstacle, Value)
)
