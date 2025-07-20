#pragma once

#include "Humble2Core.h"

// Just a POD struct
HBL2_COMPONENT(PowerUp,
{
    int Value = 50;
})

// Register members
REGISTER_HBL2_COMPONENT(PowerUp,
	HBL2_COMPONENT_MEMBER(PowerUp, Value)
)
