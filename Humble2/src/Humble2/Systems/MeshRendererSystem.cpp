#include "MeshRendererSystem.h"
#include <Core\Input.h>

namespace HBL2
{
	void MeshRendererSystem::OnCreate()
	{
		glm::mat4 mvp = glm::mat4(0.f);
		if (Context::ActiveScene->MainCamera != entt::null)
		{
			mvp = Context::ActiveScene->GetComponent<Component::Camera>(Context::ActiveScene->MainCamera).ViewProjectionMatrix;
		}

		Context::ActiveScene->GetRegistry()
			.group<Component::StaticMesh>(entt::get<Component::Transform>)
			.each([&](Component::StaticMesh& mesh, Component::Transform& transform)
			{
				if (mesh.Enabled)
				{
					transform.QRotation = glm::quat(transform.Rotation);
					transform.Matrix = glm::translate(glm::mat4(1.0f), transform.Translation) 
									* glm::toMat4(transform.QRotation) 
									* glm::scale(glm::mat4(1.0f), transform.Scale);

					Renderer3D::Get().SetupMesh(transform, mesh, mvp);
				}
			});
	}

	void MeshRendererSystem::OnUpdate(float ts)
	{
		Context::ActiveScene->GetRegistry()
			.group<Component::StaticMesh>(entt::get<Component::Transform>)
			.each([&](Component::StaticMesh& mesh, Component::Transform& transform)
			{
				if (mesh.Enabled)
				{
					if (!mesh.Static)
					{
						transform.QRotation = glm::quat(transform.Rotation);
						transform.Matrix = glm::translate(glm::mat4(1.0f), transform.Translation) 
										* glm::toMat4(transform.QRotation) 
										* glm::scale(glm::mat4(1.0f), transform.Scale);
					}

					Renderer3D::Get().SubmitMesh(transform, mesh);
				}
			});
	}
}