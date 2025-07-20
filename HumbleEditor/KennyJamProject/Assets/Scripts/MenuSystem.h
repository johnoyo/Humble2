#pragma once

#include "Humble2Core.h"

#include <imgui.h>

class MenuSystem final : public HBL2::ISystem
{
public:
	virtual void OnCreate() override
	{
		m_StartFunc = [this]() { StartGame(); };
	}

	virtual void OnUpdate(float ts) override
	{
	}

	virtual void OnGuiRender(float ts) override
	{
		if (m_GameStarted)
		{
			m_Context->View<Speedster>()
				.Each([&](Speedster& speedster)
				{
					if (!speedster.Enabled)
					{
						m_GameStarted = false;
						m_StartFunc = [this]() { RestartGame(); };
					}
				});

			return;
		}

		DrawMenu();

		if (Input::GetKeyPress(KeyCode::Num1))
		{
			m_StartFunc();
		}

		if (Input::GetKeyPress(KeyCode::Num2))
		{
			HBL2::Window::Instance->Close();
		}
	}

private:
	void DrawMenu()
	{
		UI::CreatePanel(
			UI::Config{
				.id = "SettingsUI",
				.mode = UI::Rectagle {.color = { 0, 0, 0, 0} },
				.layout = {
					.sizing =
					{
						.width = UI::Width::Grow(),
						.height = UI::Height::Grow(),
					},
					.padding = { UI::Utils::GetViewportSize().x / 4.f, UI::Utils::GetViewportSize().y / 4.f }
				}
			}, [&](UI::Panel* parent) {
				parent->AddChild(UI::Panel(UI::Config{
					.id = "OuterContainer",
					.parent = parent,
					.mode = UI::Rectagle{
						.color = { 128, 128, 128, 128 },
						.cornerRadius = 3.f,
						.borderWidth = 4.0f,
						.borderColor = { 0, 0, 0, 128 },
						.borderCornerRadius = 3.f,
					},
					.layout = UI::Layout{
						.layoutDirection = UI::LayoutDirection::TOP_TO_BOTTOM,
						.sizing = {
							.width = UI::Width::Grow(),
							.height = UI::Height::Grow(),
						},
						.padding = { 100.f, 100.f },
						.childGap = 16,
					}
					}, [&](UI::Panel* parent)
					{
						parent->AddChild(UI::Panel(UI::Config{
							.id = "TitleContainer",
							.parent = parent,
							.mode = UI::Rectagle{
								.color = { 80, 80, 80, 180 },
								.cornerRadius = 3.f,
								.borderWidth = 8.0f,
								.borderColor = { 0, 0, 0, 180 },
								.borderCornerRadius = 3.f,
							},
							.layout = UI::Layout{
								.layoutDirection = UI::LayoutDirection::LEFT_TO_RIGHT,
								.sizing = {
									.width = UI::Width::Grow(),
									.height = UI::Height::Percent(0.33f),
								},
								.padding = { 16.f, 16.f },
								.childGap = 16,
							}
							}, [&](UI::Panel* parent) {
								Text(parent, "");
								Text(parent, "");

								Text(parent, "Living Room Racer");

								Text(parent, "");
								Text(parent, "");
							})
						);

						parent->AddChild(UI::Panel(UI::Config{
							.id = "PlayContainer",
							.parent = parent,
							.mode = UI::Rectagle{
								.color = { 128, 128, 128, 180 },
								.cornerRadius = 3.f,
								.borderWidth = 4.0f,
								.borderColor = { 0, 0, 0, 180 },
								.borderCornerRadius = 3.f,
							},
							.layout = UI::Layout{
								.layoutDirection = UI::LayoutDirection::LEFT_TO_RIGHT,
								.sizing = {
									.width = UI::Width::Grow(),
									.height = UI::Height::Percent(0.33f),
								},
								.padding = { 16.f, 16.f },
								.childGap = 16,
							}
							}, [&](UI::Panel* parent) {
								Text(parent, "");
								Text(parent, "");

								Text(parent, "\n     Play (Press 1)");

								if (UI::Panel::Clicked())
								{
									m_StartFunc();
								}

								Text(parent, "");
								Text(parent, "");
							})
						);

						parent->AddChild(UI::Panel(UI::Config{
							.id = "QuitContainer",
							.parent = parent,
							.mode = UI::Rectagle{
								.color = { 128, 128, 128, 180 },
								.cornerRadius = 3.f,
								.borderWidth = 4.0f,
								.borderColor = { 0, 0, 0, 180 },
								.borderCornerRadius = 3.f,
							},
							.layout = UI::Layout{
								.layoutDirection = UI::LayoutDirection::LEFT_TO_RIGHT,
								.sizing = {
									.width = UI::Width::Grow(),
									.height = UI::Height::Percent(0.33f),
								},
								.padding = { 16.f, 16.f },
								.childGap = 16,
							}
							}, [&](UI::Panel* parent) {
								Text(parent, "");
								Text(parent, "");

								Text(parent, "\n     Quit (Press 2)");

								if (UI::Panel::Clicked())
								{
									HBL2::Window::Instance->Close();
								}

								Text(parent, "");
								Text(parent, "");
							})
						);
					})
				);
			});
	}

	void RestartGame()
	{
		// Enable car.
		m_Context->View<Speedster, Component::Transform, Component::Rigidbody>()
			.Each([](Speedster& speedster, Component::Transform& tr, Component::Rigidbody& rb)
			{
				speedster.Power = 100.f;
				speedster.Score = 0.f;
				speedster.Speed = 7.f;

				PhysicsEngine3D::Instance->SetPosition(rb, { -6.110f, 2.861f, -10.681f });
				PhysicsEngine3D::Instance->SetRotation(rb, { 0.0f, -30.f, 0.0f });

				PhysicsEngine3D::Instance->SetLinearVelocity(rb, { 0.0f, 0.f, 0.0f });
				PhysicsEngine3D::Instance->SetAngularVelocity(rb, { 0.0f, 0.f, 0.0f });

				tr.Translation = { -6.110f, 2.861f, -10.681f };
				tr.Rotation = { 0.0f, 0.f, 0.0f };

				speedster.Enabled = true;
			});

		// Enable track chunks.
		m_Context->View<TrackChunk>()
			.Each([](TrackChunk& trackChunk)
			{
				trackChunk.Enabled = true;
			});

		// Destroy all items.
		DynamicArray<Entity, BumpAllocator> itemsToDestroy = MakeDynamicArray<Entity>(&Allocator::Frame);

		m_Context->View<Item>()
			.Each([&itemsToDestroy](Entity e, Item& item)
			{
				itemsToDestroy.Add(e);
			});

		for (Entity e : itemsToDestroy)
		{
			Prefab::Destroy(e);
		}

		// Enable item manager.
		m_Context->View<ItemManager>()
			.Each([&](ItemManager& itemManager)
			{
				itemManager.Enabled = true;
				itemManager.Reset = true;
			});

		m_GameStarted = true;
	}

	void StartGame()
	{
		// Enable car.
		m_Context->View<Speedster>()
			.Each([](Speedster& speedster)
			{
				speedster.Enabled = true;
			});

		// Enable track chunks.
		m_Context->View<TrackChunk>()
			.Each([](TrackChunk& trackChunk)
			{
				trackChunk.Enabled = true;
			});

		// Enable items.
		m_Context->View<Item>()
			.Each([](Item& item)
			{
				item.Enabled = true;
			});

		m_Context->View<ItemManager>()
			.Each([&](ItemManager& itemManager)
			{
				itemManager.Enabled = true;
				itemManager.Reset = true;
			});

		m_GameStarted = true;
	}

	void Text(UI::Panel* parent, const char* text, glm::vec4 color = { 255, 255, 255, 255 })
	{
		parent->AddChild(UI::Panel{
			UI::Config{
				.id = text,
				.parent = parent,
				.mode = UI::Rectagle{
					.color = { color.r, color.g, color.b, color.a },
				},
				.layout = UI::Layout{
					.sizing = {
						.width = UI::Width::Grow(),
						.height = UI::Height::Grow()
					},
					.padding = { 20, 20 },
				},
				.type = UI::ElementType::TEXT,
			},
			[](UI::Panel* parent) { }
		});
	}

private:
	bool m_GameStarted = false;
	std::function<void()> m_StartFunc;
};

REGISTER_HBL2_SYSTEM(MenuSystem)
