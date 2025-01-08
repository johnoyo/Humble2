#pragma once

#include "Core/Window.h"

#include <imgui.h>

namespace HBL2
{
	class HBL2_API ImGuiRenderer
	{
	public:
		static ImGuiRenderer* Instance;

		virtual void Initialize() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual void Clean() = 0;

	protected:
		Window* m_Window = nullptr;
		const char* m_GlslVersion = nullptr;
	};
}