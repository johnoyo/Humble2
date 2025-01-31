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
		enum class HBL2_API ElementType
		{
			PANEL = 0,
			TEXT,
			IMAGE,
		};

		enum class HBL2_API FlexStyle
		{
			FIXED = 0,
			PERCENT,
			GROW,
		};

		struct HBL2_API Width
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

		struct HBL2_API Height
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

		enum class HBL2_API LayoutDirection
		{
			TOP_TO_BOTTOM = 0,
			LEFT_TO_RIGHT,
		};

		struct HBL2_API Rectagle
		{
			glm::vec4 color;
			float cornerRadius = 0.f;
			float borderWidth = 0.f;
			glm::vec4 borderColor = { 255, 255, 255, 255 };
			float borderCornerRadius = 0.f;
		};

		struct HBL2_API Sizing
		{
			Width width;
			Height height;
		};

		struct HBL2_API Alignment
		{
			float x;
			float y;
		};

		struct HBL2_API Layout
		{
			LayoutDirection layoutDirection = LayoutDirection::TOP_TO_BOTTOM;
			Sizing sizing;
			glm::vec2 padding = { 0, 0 };
			float childGap = 0.0f;
			Alignment childAlignment;
		};

		class Panel;

		struct HBL2_API Config
		{
			const char* id;
			Panel* parent = nullptr;
			Rectagle mode;
			Layout layout;
			ElementType type = ElementType::PANEL;
		};

		class HBL2_API Panel
		{
		public:
			Panel(Config&& config, const std::function<void(Panel*)>& body);

			Panel(const char* panelName, Config&& config, const std::function<void(Panel*)>& body);

			void AddChild(Panel&& child);

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
			const uint32_t GetChildCountThatGrowOnWidth() const;

			const uint32_t GetChildCountThatGrowOnHeight() const;

			void ComputeWidthAndHeight();

			void ComputeSizeAndPosition(ImVec2* position, ImVec2* size);

			void BeginPanel(const ImVec2& position, const ImVec2& size);

			void EndPanel();

			void Render();
			
		private:
			Config m_Configuration{};
			std::vector<Panel> m_Children;
			std::function<void(Panel*)> m_Body;
			float m_OffsetX = 0.0f;
			float m_OffsetY = 0.0f;
			bool m_Editor = false;
			const std::string& m_PanelName;
		};
	}
}