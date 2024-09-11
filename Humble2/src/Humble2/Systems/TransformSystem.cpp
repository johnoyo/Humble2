#include "TransformSystem.h"

namespace HBL2
{
	void TransformSystem::OnCreate()
	{
		m_Context->GetRegistry()
			.view<Component::Transform>()
			.each([&](Component::Transform& transform)
			{
				glm::mat4 T = glm::translate(glm::mat4(1.0f), transform.Translation);
				transform.QRotation = glm::quat({ glm::radians(transform.Rotation.x), glm::radians(transform.Rotation.y), glm::radians(transform.Rotation.z) });
				glm::mat4 S = glm::scale(glm::mat4(1.0f), transform.Scale);

				transform.Matrix = T * glm::toMat4(transform.QRotation) * S;
				transform.WorldMatrix = transform.Matrix;
			});
	}

	void TransformSystem::OnUpdate(float ts)
	{
		m_Context->GetRegistry()
			.view<Component::Transform>()
			.each([&](Component::Transform& transform)
			{
				if (!transform.Static)
				{
					glm::mat4 T = glm::translate(glm::mat4(1.0f), transform.Translation);
					transform.QRotation = glm::quat({ glm::radians(transform.Rotation.x), glm::radians(transform.Rotation.y), glm::radians(transform.Rotation.z) });
					glm::mat4 S = glm::scale(glm::mat4(1.0f), transform.Scale);

					transform.Matrix = T * glm::toMat4(transform.QRotation) * S;
					transform.WorldMatrix = transform.Matrix;
				}
			});
	}
}
