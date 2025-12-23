#pragma once

#include "ImGui\ImGuiRenderer.h"

#include <imgui.h>
#include <ImGuizmo.h>

namespace HBL2
{
	class OpenGLImGuiRenderer final : public ImGuiRenderer
	{
	public:
		virtual void Initialize() override;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;
		virtual void Render(const FrameData2& frameData) override;

		virtual void Clean() override;
	};
}