#include "RenderPassRenderer.h"

#include "Renderer.h"

namespace HBL2
{
	void RenderPassRenderer::DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws)
	{
		if (globalDraw.BindGroup.IsValid())
		{
			Renderer::Instance->WriteBuffer(globalDraw.BindGroup, 0);
		}

		draws.PerShader([&](LocalDrawStream& draw)
		{
			Renderer::Instance->WriteBuffer(draw.BindGroup, 0);
			Renderer::Instance->SetPipeline(draw.Shader);
		})
		.PerDraw([&](LocalDrawStream& draw)
		{
			Renderer::Instance->SetBuffers(draw.Mesh);
			Renderer::Instance->SetBindGroups(draw.Material);
			Renderer::Instance->SetBindGroup(draw.BindGroup, 0, draw.Offset);

			Renderer::Instance->Draw(draw.Mesh);
		})
		.Iterate();
	}
}
