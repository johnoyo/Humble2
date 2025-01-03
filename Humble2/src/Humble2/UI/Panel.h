#pragma once

#include "Base.h"

#include "UserInterfaceUtilities.h"

#include <vector>
#include <initializer_list>

#include "imgui.h"

namespace HBL2
{
	namespace UI
	{
		enum FlexStyle
		{
			FIXED = 0,
			PERCENT,
			GROW,
		};

		struct Width
		{
			static Width Fixed(float value)
			{
				return { value, FlexStyle::FIXED };
			}

			static Width Grow()
			{
				return { 0.f, FlexStyle::GROW };
			}

			static Width Percent(float percent)
			{
				return { 0.f, FlexStyle::PERCENT, percent };
			}

			float Value;
			FlexStyle Style;

			float Percentage = 0.0f;
		};

		struct Height
		{
			static Height Fixed(float value)
			{
				return { value, FlexStyle::FIXED };
			}

			static Height Grow()
			{
				return { 0.f, FlexStyle::GROW };
			}

			static Height Percent(float percent)
			{
				return { 0.f, FlexStyle::PERCENT, percent };
			}

			float Value;
			FlexStyle Style;

			float Percentage = 0.0f;
		};

		enum class LayoutDirection
		{
			TOP_TO_BOTTOM = 0,
			LEFT_TO_RIGHT,
		};

		struct Rectagle
		{
			glm::vec4 color;
			float cornerRadius = 0;
		};

		struct Sizing
		{
			Width width;
			Height height;
		};

		struct Alignment
		{
			float x;
			float y;
		};

		struct Layout
		{
			LayoutDirection layoutDirection = LayoutDirection::TOP_TO_BOTTOM;
			Sizing sizing;
			glm::vec2 padding;
			float childGap = 0.0f;
			Alignment childAlignment;
		};

		class Panel;

		struct Config
		{
			const char* id;
			Panel* parent = nullptr;
			Rectagle mode;
			Layout layout;
		};

		class Panel
		{
		public:
			Panel(Config&& config, const std::function<void(Panel*)>& body)
				: m_Configuration(std::move(config)), m_Body(body)
			{
				m_OffsetX = Utils::GetWindowPosition().x;
				m_OffsetY = Utils::GetWindowPosition().y;

				if (m_Configuration.parent == nullptr)
				{
					Render();
				}
			}

			void AddChild(Panel&& child)
			{
				m_Children.emplace_back(child);
			}

			const uint32_t GetChildCount() const
			{
				return (uint32_t)m_Children.size();
			}

			const Config& GetConfig() const
			{
				return m_Configuration;
			}

			static inline bool Clicked()
			{
				return ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
			}

		private:
			const uint32_t GetChildCountThatGrowOnWidth() const
			{
				uint32_t count = 0;

				for (const Panel& panel : m_Children)
				{
					if (panel.m_Configuration.layout.sizing.width.Style == FlexStyle::GROW)
					{
						count++;
					}
				}

				return count;
			}

			const uint32_t GetChildCountThatGrowOnHeight() const
			{
				uint32_t count = 0;

				for (const Panel& panel : m_Children)
				{
					if (panel.m_Configuration.layout.sizing.height.Style == FlexStyle::GROW)
					{
						count++;
					}
				}

				return count;
			}

			void ComputeSizing()
			{
				if (m_Configuration.parent != nullptr)
				{
					float parentWidth = m_Configuration.parent->GetConfig().layout.sizing.width.Value;
					float parentPaddingX = m_Configuration.parent->GetConfig().layout.padding.x;
					float parentChildGap = m_Configuration.parent->GetConfig().layout.childGap;
					uint32_t childCountThatContributesToGap = 0;

					switch (m_Configuration.layout.sizing.width.Style)
					{
					case FlexStyle::FIXED:
						break;

					case FlexStyle::PERCENT:
						if (m_Configuration.parent->GetConfig().layout.layoutDirection == LayoutDirection::LEFT_TO_RIGHT)
						{
							childCountThatContributesToGap = m_Configuration.parent->GetChildCount() - 1;
						}

						m_Configuration.layout.sizing.width.Value = (parentWidth - (parentChildGap * childCountThatContributesToGap) - (parentPaddingX * 2.f)) * m_Configuration.layout.sizing.width.Percentage;
						break;

					case FlexStyle::GROW:
						uint32_t childCountThatGrowOnWidth = 1;
						float sizeOccupiedByFixedWidthPanels = 0.0f;

						if (m_Configuration.parent->GetConfig().layout.layoutDirection == LayoutDirection::LEFT_TO_RIGHT)
						{
							childCountThatContributesToGap = m_Configuration.parent->GetChildCount() - 1;
							childCountThatGrowOnWidth = m_Configuration.parent->GetChildCountThatGrowOnWidth();

							for (const Panel& panel : m_Configuration.parent->m_Children)
							{
								if (panel.m_Configuration.layout.sizing.width.Style == FlexStyle::FIXED)
								{
									sizeOccupiedByFixedWidthPanels += panel.m_Configuration.layout.sizing.width.Value;
								}
								else if (panel.m_Configuration.layout.sizing.width.Style == FlexStyle::PERCENT)
								{
									// Compute panel width based on percentage in case its not yet calculated, since it defaults to 0.
									sizeOccupiedByFixedWidthPanels += (parentWidth - (parentChildGap * childCountThatContributesToGap) - (parentPaddingX * 2.f)) * panel.m_Configuration.layout.sizing.width.Percentage;
								}
							}
						}

						m_Configuration.layout.sizing.width.Value = (parentWidth - sizeOccupiedByFixedWidthPanels - (parentChildGap * childCountThatContributesToGap) - (parentPaddingX * 2.f)) / childCountThatGrowOnWidth;

						break;
					}

					float parentHeight = m_Configuration.parent->GetConfig().layout.sizing.height.Value;
					float parentPaddingY = m_Configuration.parent->GetConfig().layout.padding.y;
					childCountThatContributesToGap = 0;
					
					switch (m_Configuration.layout.sizing.height.Style)
					{
					case FlexStyle::FIXED:
						break;

					case FlexStyle::PERCENT:
						if (m_Configuration.parent->GetConfig().layout.layoutDirection == LayoutDirection::TOP_TO_BOTTOM)
						{
							childCountThatContributesToGap = m_Configuration.parent->GetChildCount() - 1;
						}

						m_Configuration.layout.sizing.height.Value = (parentHeight - (parentChildGap * childCountThatContributesToGap) - (parentPaddingY * 2.f)) * m_Configuration.layout.sizing.height.Percentage;
						break;

					case FlexStyle::GROW:
						uint32_t childCountThatGrowOnHeight = 1;
						float sizeOccupiedByFixedHeightPanels = 0.0f;

						if (m_Configuration.parent->GetConfig().layout.layoutDirection == LayoutDirection::TOP_TO_BOTTOM)
						{
							childCountThatContributesToGap = m_Configuration.parent->GetChildCount() - 1;
							childCountThatGrowOnHeight = m_Configuration.parent->GetChildCountThatGrowOnHeight();

							for (const Panel& panel : m_Configuration.parent->m_Children)
							{
								if (panel.m_Configuration.layout.sizing.height.Style == FlexStyle::FIXED)
								{
									sizeOccupiedByFixedHeightPanels += panel.m_Configuration.layout.sizing.height.Value;
								}
								else if (panel.m_Configuration.layout.sizing.height.Style == FlexStyle::PERCENT)
								{
									// Compute panel height based on percentage in case its not yet calculated, since it defaults to 0.
									sizeOccupiedByFixedHeightPanels += (parentHeight - (parentChildGap * childCountThatContributesToGap) - (parentPaddingY * 2.f)) * panel.m_Configuration.layout.sizing.height.Percentage;
								}
							}
						}

						m_Configuration.layout.sizing.height.Value = (parentHeight - sizeOccupiedByFixedHeightPanels - (parentChildGap * childCountThatContributesToGap) - (parentPaddingY * 2.f)) / childCountThatGrowOnHeight;

						break;
					}
				}
				else
				{
					switch (m_Configuration.layout.sizing.width.Style)
					{
					case FlexStyle::FIXED:
						break;

					case FlexStyle::GROW:
						m_Configuration.layout.sizing.width.Value = Utils::GetWindowSize().x;
						break;
					}

					switch (m_Configuration.layout.sizing.height.Style)
					{
					case FlexStyle::FIXED:
						break;

					case FlexStyle::GROW:
						m_Configuration.layout.sizing.height.Value = Utils::GetWindowSize().y;
						break;
					}
				}
			}

			void Render()
			{
				ComputeSizing();

				const ImVec2 windowSize = ImVec2(m_Configuration.layout.sizing.width.Value, m_Configuration.layout.sizing.height.Value);
				ImVec2 windowPos;

				if (m_Configuration.parent == nullptr)
				{
					windowPos = ImVec2(m_OffsetX, m_OffsetY);
					m_OffsetX += m_Configuration.layout.padding.x;
					m_OffsetY += m_Configuration.layout.padding.y;
				}
				else
				{
					windowPos = ImVec2(m_Configuration.parent->m_OffsetX, m_Configuration.parent->m_OffsetY);

					// Set up offsets for panel to be utilized by child nodes
					m_OffsetX = m_Configuration.parent->m_OffsetX + m_Configuration.layout.padding.x;
					m_OffsetY = m_Configuration.parent->m_OffsetY + m_Configuration.layout.padding.y;

					switch (m_Configuration.parent->m_Configuration.layout.layoutDirection)
					{
					case LayoutDirection::LEFT_TO_RIGHT:
						m_Configuration.parent->m_OffsetX +=
							m_Configuration.layout.sizing.width.Value + m_Configuration.parent->m_Configuration.layout.childGap;
						break;
					case LayoutDirection::TOP_TO_BOTTOM:
						m_Configuration.parent->m_OffsetY +=
							m_Configuration.layout.sizing.height.Value + m_Configuration.parent->m_Configuration.layout.childGap;
						break;
					}
				}

				ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
				ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

				ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, m_Configuration.mode.cornerRadius);
				ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(m_Configuration.mode.color.r, m_Configuration.mode.color.g, m_Configuration.mode.color.b, m_Configuration.mode.color.a));


				ImGui::Begin(m_Configuration.id, nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

				// This is added to prevent windows getting in front when clicked and hiding other child windows.
				ImGui::SetNextWindowFocus(); // TODO: Fix! Other panel interactions are broken because of this.

				m_Body(this);

				for (Panel& panel : m_Children)
				{
					panel.Render();
				}

				ImGui::End();

				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
			}
			
		private:
			Config m_Configuration{};
			std::vector<Panel> m_Children;
			std::function<void(Panel*)> m_Body;
			float m_OffsetX = 0.0f;
			float m_OffsetY = 0.0f;
		};
	}
}