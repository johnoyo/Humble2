#include "SpriteRendererSystem.h"
#include <glm\gtx\quaternion.hpp>

namespace HBL2
{
	void SpriteRendererSystem::OnCreate()
	{
#ifdef EMSCRIPTEN
		Shader::Create("Basic", "assets/shaders/BasicES.shader");
#else
		Shader::Create("Basic", "assets/shaders/Basic.shader");
#endif
		glm::mat4 mvp = glm::mat4(0.f);
		
		if (Context::ActiveScene->MainCamera != entt::null)
		{
			mvp = Context::ActiveScene->GetComponent<Component::Camera>(Context::ActiveScene->MainCamera).ViewProjectionMatrix;
		}

		m_BatchIndex = Renderer2D::Get().AddBatch("Basic", MAX_BATCH_SIZE, mvp);
	}

	void SpriteRendererSystem::OnUpdate(float ts)
	{
		Context::ActiveScene->GetRegistry()
			.group<Component::Sprite, Component::Transform>()
			.each([&](Component::Sprite& sprite, Component::Transform& transform)
			{
				if (sprite.Enabled)
				{
					if (!sprite.Static)
					{
						transform.QRotation = glm::quat(transform.Rotation);

						transform.Matrix = glm::translate(glm::mat4(1.0f), transform.Translation)
										* glm::toMat4(transform.QRotation)
										* glm::scale(glm::mat4(1.0f), glm::vec3(transform.Scale.x, transform.Scale.y, 1.0f));
					}

					Renderer2D::Get().DrawQuad(m_BatchIndex, transform, Texture::Get(sprite.TextureIndex)->GetSlot(), sprite.Color);
				}
			});
	}
}