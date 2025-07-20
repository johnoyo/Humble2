#pragma once

#include "Humble2Core.h"

// Just a POD struct
HBL2_COMPONENT(Coin,
{
    int Value = 1000;
})

// Register members
REGISTER_HBL2_COMPONENT(Coin,
	HBL2_COMPONENT_MEMBER(Coin, Value)
)
