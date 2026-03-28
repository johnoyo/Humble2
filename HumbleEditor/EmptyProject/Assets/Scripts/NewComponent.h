#pragma once

#include "Humble2Core.h"

// Just a POD struct
struct NewComponent
{
	int Value = 1;
	HBL2::Entity Mario = HBL2::Entity::Null;
	HBL2::Handle<HBL2::Scene> SceneHandle = {};

	static constexpr auto schema = HBL2::Reflect::Schema
	{
		HBL2::Reflect::Field{"Value", &NewComponent::Value},
		HBL2::Reflect::Field{"Mario", &NewComponent::Mario},
		HBL2::Reflect::Field{"SceneHandle", &NewComponent::SceneHandle},
	};
};

// Register members
REGISTER_HBL2_COMPONENT(NewComponent)
