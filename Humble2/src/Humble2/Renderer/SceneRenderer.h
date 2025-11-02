#pragma once

#include "Base.h"
#include "Scene/Scene.h"

#include <entt.hpp>

namespace HBL2
{
	class HBL2_API SceneRenderer
	{
	public:
		virtual void Initialize(Scene* scene) = 0;
		virtual void* Gather(Entity mainCamera) = 0;
		virtual void Render(void* renderData) = 0;
		virtual void CleanUp() = 0;

		Handle<Texture> GetTexture() const { return m_Texture; }

	protected:
		Handle<Texture> m_Texture;
		Scene* m_Scene = nullptr;
	};
}