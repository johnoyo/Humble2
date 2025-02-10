#pragma once

#include "Humble2Core.h"

// Just a POD struct
HBL2_COMPONENT(NewComponent,
{
    int Value = 1;
    int Value2 = 2;
})

// Register members
REGISTER_HBL2_COMPONENT(NewComponent,
	HBL2_COMPONENT_MEMBER(NewComponent, Value)
	HBL2_COMPONENT_MEMBER(NewComponent, Value2)
)
