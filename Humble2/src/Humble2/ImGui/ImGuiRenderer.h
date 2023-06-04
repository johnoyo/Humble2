#pragma once

#include "Core/Window.h"

#include <imgui.h>

namespace HBL2
{
	class ImGuiRenderer
	{
	public:
		ImGuiRenderer(const ImGuiRenderer&) = delete;

		static ImGuiRenderer& Get()
		{
			static ImGuiRenderer instance;
			return instance;
		}

		void Initialize(Window* window);

		void BeginFrame();
		void EndFrame();

		void Clean();

	private:
		ImGuiRenderer() {}

		Window* m_Window = nullptr;
		const char* m_GlslVersion = nullptr;
	};
}