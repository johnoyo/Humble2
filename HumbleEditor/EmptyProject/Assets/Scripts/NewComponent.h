#pragma once

#include "Humble2Core.h"

// Just a POD struct
HBL2_COMPONENT(NewComponent,
{
    int Value = 1;
	HBL2::Entity Mario = HBL2::Entity::Null;
	HBL2::Handle<HBL2::Scene> SceneHandle = {};
})

// Register members
REGISTER_HBL2_COMPONENT(NewComponent,
	HBL2_COMPONENT_MEMBER(NewComponent, Value)
	HBL2_COMPONENT_MEMBER(NewComponent, Mario)
	HBL2_COMPONENT_MEMBER(NewComponent, SceneHandle)
)
