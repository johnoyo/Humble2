#pragma once

#include "Humble2API.h"
#include "Scene\Components.h"

#include <box2d\box2d.h>

namespace HBL2
{
	namespace Physics2D
	{
		HBL2_API void ApplyLinearImpulse(Component::Rigidbody2D& rb2d, const glm::vec2& velocity, bool wake);
		HBL2_API void ApplyLinearImpulse(Component::Rigidbody2D& rb2d, const glm::vec2& velocity, const glm::vec2& worldPosition, bool wake);
		HBL2_API glm::vec2 GetLinearVelocity(Component::Rigidbody2D& rb2d);
	}
}