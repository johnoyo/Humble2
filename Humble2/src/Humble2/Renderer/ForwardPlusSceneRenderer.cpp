#include "ForwardPlusSceneRenderer.h"

namespace HBL2
{
	void ForwardPlusSceneRenderer::Initialize(Scene* scene)
	{

	}

	void ForwardPlusSceneRenderer::Gather(Entity mainCamera)
	{
		ForwardPlusSceneRenderData* sceneRenderData = &m_RenderData[Renderer::Instance->GetFrameWriteIndex()];
	}

	void ForwardPlusSceneRenderer::Render(void* renderData, void* debugRenderData)
	{
		BEGIN_PROFILE_PASS();

		ForwardPlusSceneRenderData* sceneRenderData = (ForwardPlusSceneRenderData*)renderData;

		END_PROFILE_PASS(Renderer::Instance->GetStats().MainPassTime);
	}

	void ForwardPlusSceneRenderer::CleanUp()
	{

	}

	void* ForwardPlusSceneRenderer::GetRenderData()
	{
		return &m_RenderData[Renderer::Instance->GetFrameWriteIndex()];
	}
}
