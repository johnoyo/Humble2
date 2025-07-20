#pragma once

#include "Humble2Core.h"

// Just a POD struct
HBL2_COMPONENT(ItemManager,
{
	bool Enabled = false;
	bool Reset = false;

    HBL2::UUID PowerUpPrefab;
    HBL2::UUID ObstaclePrefab;
    HBL2::UUID CoinPrefab;
})

// Register members
REGISTER_HBL2_COMPONENT(ItemManager,
	HBL2_COMPONENT_MEMBER(ItemManager, Enabled)
	HBL2_COMPONENT_MEMBER(ItemManager, PowerUpPrefab)
	HBL2_COMPONENT_MEMBER(ItemManager, ObstaclePrefab)
	HBL2_COMPONENT_MEMBER(ItemManager, CoinPrefab)
)
