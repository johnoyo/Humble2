#pragma once

#include "Humble2.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"

#include <glm\glm.hpp>
#include <glm\gtx\quaternion.hpp>

namespace HBL2
{
	namespace Editor
	{
		class TransformSystem final : public HBL2::ISystem
		{
		public:
			TransformSystem() { Name = "TransformSystem"; }

			virtual void OnCreate() override;
			virtual void OnUpdate(float ts) override;
		};
	}
}