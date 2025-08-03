#pragma once

#include "Core\Context.h"
#include "Scene\ISystem.h"
#include "Scene\Components.h"

#include <glm\glm.hpp>
#include <glm\gtx\quaternion.hpp>

namespace HBL2
{
	class HBL2_API TransformSystem final : public ISystem
	{
	public:
		TransformSystem() { Name = "TransformSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
	};
}