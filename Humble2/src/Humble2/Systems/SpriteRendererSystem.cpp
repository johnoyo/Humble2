#include "SpriteRendererSystem.h"

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
			mvp = Context::ActiveScene->GetComponent<Component::Camera>(Context::ActiveScene->MainCamera).ViewProjectionMatrix;

		m_BatchIndex = Renderer2D::Get().AddBatch("Basic", MAX_BATCH_SIZE, mvp);
	}

	void SpriteRendererSystem::OnUpdate(float ts)
	{
		Context::ActiveScene->GetRegistry()
			.group<Component::Sprite, Component::Transform>()
			.each([&](Component::Sprite& sprite, Component::Transform& transform)
			{
				if (sprite.Enabled)
					Renderer2D::Get().DrawQuad(m_BatchIndex, transform.Position, transform.Scale, 0.0f, sprite.Color);
			});
	}
}