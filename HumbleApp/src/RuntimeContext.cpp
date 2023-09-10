#include "RuntimeContext.h"

namespace HBL2Runtime
{
    void RuntimeContext::OnAttach()
    {
        ActiveScene = HBL2::Scene::Copy(EmptyScene);

#ifdef EMSCRIPTEN
		// Runtime camera setup.
		auto camera = ActiveScene->CreateEntity();
		ActiveScene->GetComponent<HBL2::Component::Tag>(camera).Name = "Camera";
		ActiveScene->AddComponent<HBL2::Component::Camera>(camera).Enabled = true;
		ActiveScene->GetComponent<HBL2::Component::Transform>(camera).Translation.z = 100.f;

		// Add a monkeh.
		auto monkeh = ActiveScene->CreateEntity();
		ActiveScene->GetComponent<HBL2::Component::Tag>(monkeh).Name = "Monkeh";
		ActiveScene->GetComponent<HBL2::Component::Transform>(monkeh).Scale = { 5.f, 5.f, 5.f };
		auto& mesh = ActiveScene->AddComponent<HBL2::Component::StaticMesh>(monkeh);
		mesh.Path = "assets/meshes/monkey_smooth.obj";
		mesh.ShaderName = "BasicMesh";
#else
		HBL2::SceneSerializer sceneSerializer(ActiveScene);
		sceneSerializer.Deserialize("assets/scenes/Example.humble");
#endif // EMSCRIPTEN
    }

    void RuntimeContext::OnCreate()
    {
		for (HBL2::ISystem* system : ActiveScene->GetSystems())
		{
			system->OnCreate();
		}
    }

    void RuntimeContext::OnUpdate(float ts)
    {
		for (HBL2::ISystem* system : ActiveScene->GetSystems())
		{
			system->OnUpdate(ts);
		}
    }

    void RuntimeContext::OnGuiRender(float ts)
    {
    }
}