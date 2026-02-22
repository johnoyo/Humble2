#include "Systems\EditorPanelSystem.h"

#include "Script\BuildEngine.h"

namespace HBL2
{
	namespace Editor
	{
		void EditorPanelSystem::DrawTrayPanel()
		{
			if (ImGui::Button("Recompile Scripts"))
			{
				if (Context::Mode == Mode::Runtime)
				{
					HBL2_WARN("Hot reloading is not available yet. Skipping requested recompilation.");
				}
				else
				{
					BuildEngine::Instance->Recompile();
				}
			}
		}
	}
}