#include "EditorContext.h"

namespace HBL2Editor
{
	void EditorContext::OnAttach()
	{
		if (m_Scenes.size() > 0)
			ActiveScene = m_Scenes[0];
		else
		{
			// No other scene exists in project, so create a new one from the empty scene.
			ActiveScene = HBL2::Scene::Copy(EmptyScene);

			// Attach editor systems.
			ActiveScene->RegisterSystem(new EditorPanelSystem);
			ActiveScene->RegisterSystem(new EditorCameraSystem);
		}

		// Editor camera set up.
		auto editorCameraEntity = ActiveScene->CreateEntity();
		ActiveScene->GetComponent<HBL2::Component::Tag>(editorCameraEntity).Name = "EditorCamera";
		ActiveScene->AddComponent<Component::EditorCamera>(editorCameraEntity);
		ActiveScene->AddComponent<HBL2::Component::Camera>(editorCameraEntity).Enabled = true;
		ActiveScene->GetComponent<HBL2::Component::Transform>(editorCameraEntity).Position.z = 100.f;
	}

	void EditorContext::OnCreate()
	{
		for (HBL2::ISystem* system : ActiveScene->GetSystems())
		{
			system->OnCreate();
		}
	}

	void EditorContext::OnUpdate(float ts)
	{
		for (HBL2::ISystem* system : ActiveScene->GetSystems())
		{
			system->OnUpdate(ts);
		}
	}

	void EditorContext::OnGuiRender(float ts)
	{
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());

		for (HBL2::ISystem* system : ActiveScene->GetSystems())
		{
			system->OnGuiRender(ts);
		}
	}
}