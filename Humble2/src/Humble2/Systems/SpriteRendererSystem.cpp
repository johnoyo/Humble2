#include "SpriteRendererSystem.h"
#include <glm\gtx\quaternion.hpp>

namespace HBL2
{
	void SpriteRendererSystem::OnCreate()
	{
#ifdef EMSCRIPTEN
		HBL::Shader::Create("Basic", "assets/shaders/BasicES.shader");
#else
		HBL::Shader::Create("Basic", "assets/shaders/Basic.shader");
#endif
		glm::mat4 mvp = glm::mat4(0.f);
		
		if (Context::Mode == Mode::Runtime)
		{
			if (Context::ActiveScene->MainCamera != entt::null)
			{
				mvp = Context::ActiveScene->GetComponent<Component::Camera>(Context::ActiveScene->MainCamera).ViewProjectionMatrix;
			}
			else
			{
				HBL2_CORE_WARN("No main camera set for runtime context.");
			}
		}
		else if (Context::Mode == Mode::Editor)
		{
			if (Context::EditorScene->MainCamera != entt::null)
			{
				mvp = Context::EditorScene->GetComponent<Component::Camera>(Context::EditorScene->MainCamera).ViewProjectionMatrix;
			}
			else
			{
				HBL2_CORE_WARN("No main camera set for editor context.");
			}
		}
		else
		{
			HBL2_CORE_WARN("No mode set for current context.");
		}

		m_BatchIndex = Renderer2D::Get().AddBatch("Basic", MAX_BATCH_SIZE, mvp);

		Context::ActiveScene->GetRegistry()
			.group<Component::Sprite, Component::Transform>()
			.each([&](Component::Sprite& sprite, Component::Transform& transform)
				{
					if (sprite.Enabled)
					{
						transform.QRotation = glm::quat({ glm::radians(transform.Rotation.x), glm::radians(transform.Rotation.y), glm::radians(transform.Rotation.z) });

						transform.Matrix = glm::translate(glm::mat4(1.0f), transform.Translation)
							* glm::toMat4(transform.QRotation)
							* glm::scale(glm::mat4(1.0f), glm::vec3(transform.Scale.x, transform.Scale.y, 1.0f));

						Renderer2D::Get().DrawQuad(m_BatchIndex, transform, (float)HBL::Texture::Get(sprite.Path)->GetSlot(), sprite.Color);
					}
				});
	}

	void SpriteRendererSystem::OnUpdate(float ts)
	{
		Context::ActiveScene->GetRegistry()
			.group<Component::Sprite, Component::Transform>()
			.each([&](Component::Sprite& sprite, Component::Transform& transform)
			{
				if (sprite.Enabled)
				{
					if (!transform.Static)
					{
						transform.QRotation = glm::quat({ glm::radians(transform.Rotation.x), glm::radians(transform.Rotation.y), glm::radians(transform.Rotation.z) });

						transform.Matrix = glm::translate(glm::mat4(1.0f), transform.Translation)
										* glm::toMat4(transform.QRotation)
										* glm::scale(glm::mat4(1.0f), glm::vec3(transform.Scale.x, transform.Scale.y, 1.0f));
					}

					Renderer2D::Get().DrawQuad(m_BatchIndex, transform, (float)HBL::Texture::Get(sprite.Path)->GetSlot(), sprite.Color);
				}
			});
	}
}