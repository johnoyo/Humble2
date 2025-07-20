#pragma once

#include "Humble2Core.h"

// Just a POD struct
HBL2_COMPONENT(Item,
{
	bool Enabled = true;
})

// Register members
REGISTER_HBL2_COMPONENT(Item,
	HBL2_COMPONENT_MEMBER(Item, Enabled)
)
