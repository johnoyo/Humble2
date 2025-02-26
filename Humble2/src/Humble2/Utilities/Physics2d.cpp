#include "Physics2d.h"

namespace HBL2
{
	void Physics2D::ApplyLinearImpulse(Component::Rigidbody2D& rb2d, const glm::vec2& velocity, bool wake)
	{
		b2Body_ApplyLinearImpulseToCenter(rb2d.BodyId, { velocity.x, velocity.y }, wake);
	}

	void Physics2D::ApplyLinearImpulse(Component::Rigidbody2D& rb2d, const glm::vec2& velocity, const glm::vec2& worldPosition, bool wake)
	{
		b2Body_ApplyLinearImpulse(rb2d.BodyId, { velocity.x, velocity.y }, { worldPosition.x, worldPosition.y }, wake);
	}

	glm::vec2 Physics2D::GetLinearVelocity(Component::Rigidbody2D& rb2d)
	{
		const auto& linearVelocity = b2Body_GetLinearVelocity(rb2d.BodyId);
		return { linearVelocity.x, linearVelocity.y };
	}
}
