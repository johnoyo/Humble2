#include "Systems\EditorPanelSystem.h"

namespace HBL2
{
	namespace Editor
	{
		void EditorPanelSystem::DrawStatsPanel(float ts)
		{
			ImGui::Text("Frame Time: %f", ts);
		}
	}
}