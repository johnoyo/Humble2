#pragma once

#include "ECS/Entity.h"

namespace HBL2
{
	namespace EntityPreset
	{
		HBL2_API Entity CreateEmpty();

		HBL2_API Entity CreateCamera();

		HBL2_API Entity CreateLight();

		HBL2_API Entity CreateSkyLight();

		HBL2_API Entity CreatePlane();

		HBL2_API Entity CreateTessellatedPlane();

		HBL2_API Entity CreateCube();

		HBL2_API Entity CreateSphere();

		HBL2_API Entity CreateCapsule();

		HBL2_API Entity CreateCylinder();

		HBL2_API Entity CreateTorus();
	
		HBL2_API Entity CreateSprite();

		HBL2_API Entity CreateTerrain();
	}
}