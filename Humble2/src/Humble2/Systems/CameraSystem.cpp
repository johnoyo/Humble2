#include "CameraSystem.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace HBL2
{
	void CameraSystem::OnCreate()
	{
		Context::ActiveScene->GetRegistry()
			.group<Component::Camera>(entt::get<Component::Transform>)
			.each([&](entt::entity entity, Component::Camera& camera, Component::Transform& transform)
			{
				if (camera.Enabled)
				{
					transform.QRotation = glm::quat(transform.Rotation);

					transform.Matrix = glm::translate(glm::mat4(1.0f), transform.Translation) * glm::toMat4(transform.QRotation);

					camera.View = glm::inverse(transform.Matrix);

					camera.ViewProjectionMatrix = camera.Projection * camera.View;

					if (camera.Primary)
						Context::ActiveScene->MainCamera = entity;
				}
			});
	}

	void CameraSystem::OnUpdate(float ts)
	{
		Context::ActiveScene->GetRegistry()
			.group<Component::Camera>(entt::get<Component::Transform>)
			.each([&](entt::entity entity, Component::Camera& camera, Component::Transform& transform)
			{
				if (camera.Enabled)
				{
					if (!camera.Static)
					{
						transform.QRotation = glm::quat(transform.Rotation);

						transform.Matrix = glm::translate(glm::mat4(1.0f), transform.Translation) * glm::toMat4(transform.QRotation);

						camera.View = glm::inverse(transform.Matrix);

						camera.ViewProjectionMatrix = camera.Projection * camera.View;

						if (camera.Primary)
							Context::ActiveScene->MainCamera = entity;
					}
					else if (camera.Static)
					{
						if (camera.Primary)
							Context::ActiveScene->MainCamera = entity;
					}
				}
			});
	}
}