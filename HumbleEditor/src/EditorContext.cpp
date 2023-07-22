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

#ifndef EMSCRIPTEN
			// Attach editor systems.
			ActiveScene->RegisterSystem(new EditorPanelSystem);
			ActiveScene->RegisterSystem(new EditorCameraSystem);
#endif
		}
#ifndef EMSCRIPTEN
		// Editor camera set up.
		auto editorCameraEntity = ActiveScene->CreateEntity();
		ActiveScene->GetComponent<HBL2::Component::Tag>(editorCameraEntity).Name = "EditorCamera";
		ActiveScene->AddComponent<Component::EditorCamera>(editorCameraEntity);
		ActiveScene->AddComponent<HBL2::Component::Camera>(editorCameraEntity).Enabled = true;
		ActiveScene->GetComponent<HBL2::Component::Transform>(editorCameraEntity).Position.z = 100.f;
#endif
		// Runtime camera setup.
		auto camera = ActiveScene->CreateEntity();
		ActiveScene->GetComponent<HBL2::Component::Tag>(camera).Name = "Camera";
#ifndef EMSCRIPTEN
		ActiveScene->AddComponent<Component::EditorVisible>(camera);
#endif
		ActiveScene->AddComponent<HBL2::Component::Camera>(camera).Enabled = true;
		ActiveScene->GetComponent<HBL2::Component::Transform>(camera).Position.z = 100.f;

		// Add a monkeh.
		auto monkeh = ActiveScene->CreateEntity();
		ActiveScene->GetComponent<HBL2::Component::Tag>(monkeh).Name = "Monkeh";
		ActiveScene->GetComponent<HBL2::Component::Transform>(monkeh).Scale = { 5.f, 5.f, 5.f };
#ifndef EMSCRIPTEN
		ActiveScene->AddComponent<Component::EditorVisible>(monkeh);
#endif
		auto& mesh = ActiveScene->AddComponent<HBL2::Component::Mesh>(monkeh);
		mesh.Path = "assets/meshes/monkey_smooth.obj";
		mesh.ShaderName = "BasicMesh";
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
#ifndef EMSCRIPTEN
		ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
#endif
		for (HBL2::ISystem* system : ActiveScene->GetSystems())
		{
			system->OnGuiRender(ts);
		}
	}
}