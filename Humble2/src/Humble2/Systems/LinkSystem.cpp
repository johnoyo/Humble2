#include "LinkSystem.h"

#include "UI\LayoutLib.h"

namespace HBL2
{
	void LinkSystem::OnCreate()
	{
		m_Context->GetRegistry()
			.group<Component::Link>(entt::get<Component::Transform>)
			.each([&](entt::entity entity, Component::Link& link, Component::Transform& transform)
			{
				transform.WorldMatrix = GetWorldSpaceTransform(entity, link);
			});
	}

	void LinkSystem::OnUpdate(float ts)
	{
		m_Context->GetRegistry()
			.group<Component::Link>(entt::get<Component::Transform>)
			.each([&](entt::entity entity, Component::Link& link, Component::Transform& transform)
			{
				if (!transform.Static)
				{
					transform.WorldMatrix = GetWorldSpaceTransform(entity, link);
				}
			});
	}

	void LinkSystem::OnGuiRender(float ts)
	{
#ifdef false
		UI::Panel(
			UI::Config{
				.id = "SettingsUI",
				.mode = UI::Rectagle { .color = {0, 0, 0, 0} },
				.layout = {
					.sizing =
					{
						.width = UI::Width::Grow(),
						.height = UI::Height::Grow(),
					},
					.padding = { UI::Utils::GetViewportSize().x / 4.f, UI::Utils::GetViewportSize().y / 4.f }
				}
			}, [&](UI::Panel* parent)
			{
				parent->AddChild(UI::Panel(UI::Config{
					.id = "OuterContainer",
					.parent = parent,
					.mode = UI::Rectagle{
						.color = { 43, 41, 51, 255 },
						.borderWidth = 4.f,
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
											.padding = { 8, 8 },
										}
									}, [&](UI::Panel* parent) {
										UI::Text(parent, "Sound");

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
											.padding = { 8, 8 },
										}
									}, [&](UI::Panel* parent) {
										UI::Text(parent, "Graphics");

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

						UI::Text(parent, "separator");

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
								.padding = { 4, 4 },
								.childGap = 4,
							}
							}, [&](UI::Panel* parent) {
								if (m_SoundTabClicked)
								{
									UI::Text(parent, "InnerContainer text 1");
								}
								else if (m_GraphicsTabClicked)
								{
									UI::Text(parent, "InnerContainer text 2");
								}
							}
						));
					}
				));
			});

		//HBL2_CORE_INFO("UI DONE");
#endif
	}

	glm::mat4 LinkSystem::GetWorldSpaceTransform(entt::entity entity, Component::Link& link)
	{
		glm::mat4 transform = glm::mat4(1.0f);

		if (link.Parent != 0)
		{
			entt::entity parentEntity = m_Context->FindEntityByUUID(link.Parent);
			if (parentEntity != entt::null)
			{
				Component::Link& parentLink = m_Context->GetComponent<Component::Link>(parentEntity);
				transform = GetWorldSpaceTransform(parentEntity, parentLink);
			}
		}

		return transform * m_Context->GetComponent<Component::Transform>(entity).LocalMatrix;
	}
}
