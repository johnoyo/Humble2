#pragma once

#include "Humble2Core.h"

// Just a POD struct
HBL2_COMPONENT(TrackChunk,
{
	bool Enabled = true;
	glm::vec3 SpawnPosition = glm::vec3(0.0f);
	glm::vec3 InitialPosition = glm::vec3(0.0f);
	glm::vec3 PositionToReset = glm::vec3(0.0f);
})

// Register members
REGISTER_HBL2_COMPONENT(TrackChunk,
	HBL2_COMPONENT_MEMBER(TrackChunk, Enabled)
)
