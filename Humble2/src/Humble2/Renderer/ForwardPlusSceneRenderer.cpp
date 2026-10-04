#include "ForwardPlusSceneRenderer.h"

namespace HBL2
{
	void ForwardPlusSceneRenderer::Initialize(Scene* scene, uint32_t maxLights)
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

		// Shadow pre-pass
		
		// Depth pre-pass

		// Grid Frustums Compute Shader
		// RWStructuredBuffer<Frustum> out_Frustums : register(u0);

		// Light Culling Compute Shader
		// Texture2D DepthTextureVS : register(t3);
		// StructuredBuffer<Frustum> in_Frustums : register(t9);

		// Geometry pass

		// Post process pass

		// Debug pass

		// Present pass

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
