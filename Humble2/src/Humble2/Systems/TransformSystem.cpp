#include "TransformSystem.h"

namespace HBL2
{
	void TransformSystem::OnCreate()
	{
		m_Context->View<Component::Transform>()
			.Each([&](Component::Transform& transform)
			{
				glm::mat4 T = glm::translate(glm::mat4(1.0f), transform.Translation);
				transform.QRotation = glm::quat({ glm::radians(transform.Rotation.x), glm::radians(transform.Rotation.y), glm::radians(transform.Rotation.z) });
				glm::mat4 S = glm::scale(glm::mat4(1.0f), transform.Scale);

				transform.LocalMatrix = T * glm::toMat4(transform.QRotation) * S;
				transform.WorldMatrix = transform.LocalMatrix;
			});
	}

	void TransformSystem::OnUpdate(float ts)
	{
		BEGIN_PROFILE_SYSTEM();

		m_Context->View<Component::Transform>()
			.Each([&](Component::Transform& transform)
			{
				if (!transform.Static)
				{
					glm::mat4 T = glm::translate(glm::mat4(1.0f), transform.Translation);
					transform.QRotation = glm::quat({ glm::radians(transform.Rotation.x), glm::radians(transform.Rotation.y), glm::radians(transform.Rotation.z) });
					glm::mat4 S = glm::scale(glm::mat4(1.0f), transform.Scale);

					transform.LocalMatrix = T * glm::toMat4(transform.QRotation) * S;
					transform.WorldMatrix = transform.LocalMatrix;
				}
			});

		END_PROFILE_SYSTEM(RunningTime);
	}
}
