#pragma once

#include "Base.h"
#include "Scene\ISystem.h"
#include "Core\Context.h"
#include "Resources\ResourceManager.h"
#include "Renderer\ForwardSceneRenderer.h"

#include <entt.hpp>

namespace HBL2
{
	class RenderingSystem final : public ISystem
	{
	public:
		RenderingSystem() { Name = "RenderingSystem"; }

		virtual void OnCreate() override;
		virtual void OnUpdate(float ts) override;
		virtual void OnDestroy() override;

	private:
		entt::entity GetMainCamera();

	private:
		ResourceManager* m_ResourceManager = nullptr;
		Scene* m_EditorScene = nullptr;
		SceneRenderer* m_SceneRenderer = nullptr;
	};
}