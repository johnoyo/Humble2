#include "ImGuiRenderer.h"

namespace HBL2
{
	ImGuiRenderer* ImGuiRenderer::Instance = nullptr;

	ImGuiContext* ImGuiRenderer::GetContext()
	{
		return m_ImGuiContext;
	}
}