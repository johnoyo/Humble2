#include "Systems\EditorPanelSystem.h"

#include "Physics/PhysicsEngine3D.h"

namespace HBL2
{
	namespace Editor
	{
		void EditorPanelSystem::DrawPlayStopPanel()
		{
			static bool isPlaying = false;

			if (ImGui::Button("Play"))
			{
				Context::Mode = Mode::Runtime;

				if (!m_ActiveSceneTemp.IsValid())
				{
					m_ActiveSceneTemp = Context::ActiveScene;
					auto playSceneHandle = ResourceManager::Instance->CreateScene({ .name = m_ActiveScene->GetName() + "(Clone)" });
					Scene* playScene = ResourceManager::Instance->GetScene(playSceneHandle);
					Scene::Copy(m_ActiveScene, playScene);
					SceneManager::Get().LoadPlaymodeScene(playSceneHandle, true);
				}

				isPlaying = true;
			}

			ImGui::SameLine();

			if (ImGui::Button("Stop"))
			{
				Context::Mode = Mode::Editor;

				if (m_ActiveSceneTemp.IsValid())
				{
					SceneManager::Get().LoadPlaymodeScene(m_ActiveSceneTemp, false);
					m_ActiveSceneTemp = {};
				}

				isPlaying = false;
			}

			ImGui::SameLine();

			if (isPlaying)
			{
				ImGui::Text("Playing ... ");
			}
			else
			{
				ImGui::Text("Editing ... ");
			}

			ImGui::SameLine();

			static bool showPhysicsColliders = false;
			ImGui::Checkbox("Show Physics Colliders", &showPhysicsColliders);

			if (PhysicsEngine3D::Instance != nullptr)
			{
				PhysicsEngine3D::Instance->SetDebugDrawEnabled(showPhysicsColliders);
			}
		}
	}
}