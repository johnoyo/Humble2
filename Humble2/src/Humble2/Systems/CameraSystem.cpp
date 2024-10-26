#include "CameraSystem.h"

#include "Renderer\Renderer.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace HBL2
{
	void CameraSystem::OnCreate()
	{
		m_Context->GetRegistry()
			.group<Component::Camera>(entt::get<Component::Transform>)
			.each([&](entt::entity entity, Component::Camera& camera, Component::Transform& transform)
			{
				if (camera.Enabled)
				{
					if (camera.Type == Component::Camera::Type::Perspective)
					{
						camera.Projection = glm::perspective(glm::radians(camera.Fov), camera.AspectRatio, camera.Near, camera.Far);
					}
					else
					{
						camera.Projection = glm::ortho(-camera.AspectRatio * camera.ZoomLevel, camera.AspectRatio * camera.ZoomLevel, -camera.ZoomLevel, camera.ZoomLevel, -1.f, 1.f);
					}

					if (Renderer::Instance->GetAPI() != GraphicsAPI::OPENGL)
					{
						camera.Projection[1][1] *= -1;
					}

					camera.View = glm::inverse(transform.WorldMatrix);
					camera.ViewProjectionMatrix = camera.Projection * camera.View;

					if (camera.Primary)
					{
						m_Context->MainCamera = entity;
					}
				}
			});
	}

	void CameraSystem::OnUpdate(float ts)
	{
		m_Context->GetRegistry()
			.group<Component::Camera>(entt::get<Component::Transform>)
			.each([&](entt::entity entity, Component::Camera& camera, Component::Transform& transform)
			{
				if (camera.Enabled)
				{
					if (!transform.Static)
					{
						if (camera.Type == Component::Camera::Type::Perspective)
						{
							camera.Projection = glm::perspective(glm::radians(camera.Fov), camera.AspectRatio, camera.Near, camera.Far);
						}
						else
						{
							camera.Projection = glm::ortho(-camera.AspectRatio * camera.ZoomLevel, camera.AspectRatio * camera.ZoomLevel, -camera.ZoomLevel, camera.ZoomLevel, -1.f, 1.f);
						}

						if (Renderer::Instance->GetAPI() != GraphicsAPI::OPENGL)
						{
							camera.Projection[1][1] *= -1;
						}

						camera.View = glm::inverse(transform.WorldMatrix);
						camera.ViewProjectionMatrix = camera.Projection * camera.View;
					}

					if (camera.Primary)
					{
						m_Context->MainCamera = entity;
					}
				}
			});
	}
}