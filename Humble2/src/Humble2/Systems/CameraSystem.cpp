#include "CameraSystem.h"

#include "Renderer\Renderer.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace HBL2
{
	void CameraSystem::OnAttach()
	{
		m_Context->Group<Component::Camera>(Get<Component::Transform>)
			.Each([this](Entity entity, Component::Camera& camera, Component::Transform& transform)
			{
				if (camera.Enabled)
				{
					if (camera.Primary)
					{
						m_Context->MainCamera = entity;
						CalculateFrustum(camera);
					}
				}
			});
	}

	void CameraSystem::OnCreate()
	{
		m_Context->Group<Component::Camera>(Get<Component::Transform>)
			.Each([](Entity entity, Component::Camera& camera, Component::Transform& transform)
			{
				if (camera.Enabled)
				{
					if (camera.Type == Component::Camera::EType::Perspective)
					{
						camera.Projection = glm::perspectiveRH_ZO(glm::radians(camera.Fov), camera.AspectRatio, camera.Near, camera.Far);
					}
					else
					{
						float extent = camera.AspectRatio * camera.ZoomLevel;
						camera.Projection = glm::orthoRH_ZO(-extent, extent, -extent, extent, 0.f, 1.f);
					}

					camera.View = glm::inverse(transform.WorldMatrix);
					camera.ViewProjectionMatrix = camera.Projection * camera.View;
				}
			});
	}

	void CameraSystem::OnUpdate(float ts)
	{
		BEGIN_PROFILE_SYSTEM();

		m_Context->Group<Component::Camera>(Get<Component::Transform>)
			.Each([this](Entity entity, Component::Camera& camera, Component::Transform& transform)
			{
				if (camera.Enabled)
				{
					if (!transform.Static)
					{
						if (camera.Type == Component::Camera::EType::Perspective)
						{
							camera.Projection = glm::perspectiveRH_ZO(glm::radians(camera.Fov), camera.AspectRatio, camera.Near, camera.Far);
						}
						else
						{
							float extent = camera.AspectRatio * camera.ZoomLevel;
							camera.Projection = glm::orthoRH_ZO(-extent, extent, -extent, extent, 0.f, 1.f);
						}

						camera.View = glm::inverse(transform.WorldMatrix);
						camera.ViewProjectionMatrix = camera.Projection * camera.View;
					}

					if (camera.Primary)
					{
						m_Context->MainCamera = entity;
						CalculateFrustum(camera);
					}
				}
			});

		END_PROFILE_SYSTEM(RunningTime);
	}

	void CameraSystem::CalculateFrustum(Component::Camera& camera)
	{
		const glm::mat4& vp = camera.ViewProjectionMatrix;

		// Left plane
		camera.Frustum.Planes[0].normal = glm::vec3(vp[0][3] + vp[0][0], vp[1][3] + vp[1][0], vp[2][3] + vp[2][0]);
		camera.Frustum.Planes[0].distance = vp[3][3] + vp[3][0];

		// Right plane
		camera.Frustum.Planes[1].normal = glm::vec3(vp[0][3] - vp[0][0], vp[1][3] - vp[1][0], vp[2][3] - vp[2][0]);
		camera.Frustum.Planes[1].distance = vp[3][3] - vp[3][0];

		// Bottom plane
		camera.Frustum.Planes[2].normal = glm::vec3(vp[0][3] + vp[0][1], vp[1][3] + vp[1][1], vp[2][3] + vp[2][1]);
		camera.Frustum.Planes[2].distance = vp[3][3] + vp[3][1];

		// Top plane
		camera.Frustum.Planes[3].normal = glm::vec3(vp[0][3] - vp[0][1], vp[1][3] - vp[1][1], vp[2][3] - vp[2][1]);
		camera.Frustum.Planes[3].distance = vp[3][3] - vp[3][1];

		// Near plane
		camera.Frustum.Planes[4].normal = glm::vec3(vp[0][3] + vp[0][2], vp[1][3] + vp[1][2], vp[2][3] + vp[2][2]);
		camera.Frustum.Planes[4].distance = vp[3][3] + vp[3][2];

		// Far plane
		camera.Frustum.Planes[5].normal = glm::vec3(vp[0][3] - vp[0][2], vp[1][3] - vp[1][2], vp[2][3] - vp[2][2]);
		camera.Frustum.Planes[5].distance = vp[3][3] - vp[3][2];

		// Normalize the planes
		for (auto& plane : camera.Frustum.Planes)
		{
			float length = glm::length(plane.normal);
			plane.normal /= length;
			plane.distance /= length;
		}
	}
}