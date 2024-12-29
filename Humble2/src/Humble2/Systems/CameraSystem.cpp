#include "CameraSystem.h"

#include "Renderer\Renderer.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "UI\LayoutLib.h"

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

					/*if (Renderer::Instance->GetAPI() != GraphicsAPI::OPENGL)
					{
						camera.Projection[1][1] *= -1;
					}*/

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

						/*if (Renderer::Instance->GetAPI() != GraphicsAPI::OPENGL)
						{
							camera.Projection[1][1] *= -1;
						}*/

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

	void CameraSystem::OnGuiRender(float ts)
	{
		UI::UserInterface(UI::Configuration{
			.ID = "OuterContainer",
			.mode = UI::Rectagle{
				.color = { 43, 41, 51, 255 },
			},
			.layout = UI::Layout{
				.sizing = {
					.width = 400,
					.height = 400
				},
				.padding = { 16, 16 },
			}
		}, UI::Body{
			CONTENTS {
				//UI::Text("OuterContainer text 1", { .color = { 255, 0, 0, 255 } });
				//UI::Text("OuterContainer text 2");

				UI::UserInterface(UI::Configuration{
					.ID = "HeaderBar",
					.mode = UI::Rectagle{
						.color = { 90, 90, 90, 255 },
						.cornerRadius = 8.0f,
					},
					.layout = UI::Layout{
						.sizing = {
							.width = 400 - 32,
							.height = 60
						},
					}
				}, UI::Body{
					CONTENTS {
						//UI::Text("InnerContainer text 1");
						//UI::Text("InnerContainer text 2");
					}
				});

				UI::UserInterface(UI::Configuration{
					.ID = "HeaderBar2",
						.mode = UI::Rectagle{
							.color = { 120, 120, 120, 255 },
							.cornerRadius = 8.0f,
					},
					.layout = UI::Layout{
						.sizing = {
							.width = 400 - 32,
							.height = 60
						},
					}
				}, UI::Body{
					CONTENTS {
						//UI::Text("InnerContainer text 1");
						//UI::Text("InnerContainer text 2");
					}
				});

				UI::UserInterface(UI::Configuration{
					.ID = "HeaderBar3",
						.mode = UI::Rectagle{
							.color = { 180, 180, 180, 255 },
							.cornerRadius = 8.0f,
					},
					.layout = UI::Layout{
						.sizing = {
							.width = 400 - 32,
							.height = 60
						},
					}
				}, UI::Body{
					CONTENTS {
						//UI::Text("InnerContainer text 1");
						//UI::Text("InnerContainer text 2");
					}
				});

				//UI::UserInterface(UI::Configuration{
				//	.ID = "LowerContent",
				//		.mode = UI::Rectagle{
				//			.color = { 90, 90, 90, 255 },
				//			.cornerRadius = 8.0f,
				//	},
				//	.layout = UI::Layout{
				//		.sizing = {
				//			.width = 200,
				//			.height = 60
				//		},
				//	}
				//}, UI::Body{
				//	CONTENTS {
				//		//UI::Text("InnerContainer text 1");
				//		//UI::Text("InnerContainer text 2");
				//	}
				//});
			}
		}).Invalidate();

		HBL2_CORE_INFO("UI DONE");
	}
}