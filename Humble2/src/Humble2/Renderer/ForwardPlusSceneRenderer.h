#pragma once

#include "SceneRenderer.h"

#include "DrawList.h"
#include "UniformRingBuffer.h"

#include "Renderer/Renderer.h"
#include "Resources/ResourceManager.h"

namespace HBL2
{
	struct ForwardPlusSceneRenderData
	{
	};

	class HBL2_API ForwardPlusSceneRenderer final : public SceneRenderer
	{
	public:
		virtual ~ForwardPlusSceneRenderer() = default;

		virtual void Initialize(Scene* scene, uint32_t maxLights) override;
		virtual void Gather(Entity mainCamera) override;
		virtual void Render(void* renderData, void* debugRenderData) override;
		virtual void CleanUp() override;

		virtual void* GetRenderData() override;

	private:
		PoolReservation* m_Reservation = nullptr;
		Arena m_Arena;

		ResourceManager* m_ResourceManager = nullptr;
		UniformRingBuffer* m_UniformRingBuffer = nullptr;

		Scene* m_EditorScene = nullptr;
		ForwardPlusSceneRenderData m_RenderData[Renderer::FrameCount]{};
	};
}