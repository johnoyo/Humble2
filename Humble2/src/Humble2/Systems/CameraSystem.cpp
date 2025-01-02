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
#ifdef false
		UI::Panel(
			UI::Config{
				.id = "SettingsUI",
				.mode = UI::Rectagle {.color = {0, 0, 0, 50} },
				.layout = {
					.sizing =
					{
						.width = UI::Width::Grow(),
						.height = UI::Height::Grow(),
					},
					.padding = { UI::Utils::GetWindowSize().x / 4.f, UI::Utils::GetWindowSize().y / 4.f }
				}
			}, [&](UI::Panel* parent)
			{
				parent->AddChild(UI::Panel(UI::Config{
					.id = "OuterContainer",
					.parent = parent,
					.mode = UI::Rectagle{
						.color = { 43, 41, 51, 255 },
					},
					.layout = UI::Layout{
						.layoutDirection = UI::LayoutDirection::TOP_TO_BOTTOM,
						.sizing = {
							.width = UI::Width::Grow(),
							.height = UI::Height::Grow(),
						},
						.padding = { 16, 16 },
						.childGap = 16,
					}
					}, [&](UI::Panel* parent) {
						parent->AddChild(UI::Panel(UI::Config{
							.id = "HeaderBar",
							.parent = parent,
							.mode = UI::Rectagle{
								.color = { 90, 90, 90, 255 },
								.cornerRadius = 8.0f,
							},
							.layout = UI::Layout{
								.layoutDirection = UI::LayoutDirection::LEFT_TO_RIGHT,
								.sizing = {
									.width = UI::Width::Grow(),
									.height = UI::Height::Fixed(44.f)
								},
								.padding = { 4, 4 },
								.childGap = 4.0f,
							}
							}, [&](UI::Panel* parent) {
								parent->AddChild(UI::Panel(UI::Config{
										.id = "SoundTab",
										.parent = parent,
										.mode = UI::Rectagle{
											.color = { 255, 90, 90, 255 },
											.cornerRadius = 8.0f,
										},
										.layout = UI::Layout{
											.sizing = {
												.width = UI::Width::Fixed(72.f),
												.height = UI::Height::Fixed(36.f),
											},
										}
									}, [&](UI::Panel* parent) {
										UI::Text("Sound");

										if (UI::Panel::Clicked())
										{
											HBL2_CORE_INFO("soundTab clicked");
											m_SoundTabClicked = true;
											m_GraphicsTabClicked = false;
										}
									}
								));

								parent->AddChild(UI::Panel(UI::Config{
										.id = "GraphicsTab",
										.parent = parent,
										.mode = UI::Rectagle{
											.color = { 255, 90, 90, 255 },
											.cornerRadius = 8.0f,
										},
										.layout = UI::Layout{
											.sizing = {
												.width = UI::Width::Fixed(72.f),
												.height = UI::Height::Fixed(36.f),
											},
										}
									}, [&](UI::Panel* parent) {
										UI::Text("Graphics");

										if (UI::Panel::Clicked())
										{
											HBL2_CORE_INFO("graphicsTab clicked");
											m_GraphicsTabClicked = true;
											m_SoundTabClicked = false;
										}
									}
								));
							}
						));

						parent->AddChild(UI::Panel(UI::Config{
							.id = "Content",
							.parent = parent,
							.mode = UI::Rectagle{
								.color = { 120, 120, 120, 255 },
								.cornerRadius = 8.0f,
							},
							.layout = UI::Layout{
								.sizing = {
									.width = UI::Width::Grow(),
									.height = UI::Height::Grow(),
								},
							}
							}, [&](UI::Panel* parent) {
								if (m_SoundTabClicked)
								{
									UI::Text("InnerContainer text 1");
								}
								else if (m_GraphicsTabClicked)
								{
									UI::Text("InnerContainer text 2");
								}
							}
						));
					}
				));				
			});

			//HBL2_CORE_INFO("UI DONE");
#endif
	}
}