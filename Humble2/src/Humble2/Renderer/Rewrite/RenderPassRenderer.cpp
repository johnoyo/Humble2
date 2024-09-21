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
			Renderer::Instance->SetBuffers(draw.Mesh, draw.Material);
			Renderer::Instance->SetBindGroups(draw.Material);
			Renderer::Instance->SetBindGroup(draw.BindGroup, 0, draw.Offset);

			Mesh* openGLMesh = ResourceManager::Instance->GetMesh(draw.Mesh);

			if (openGLMesh->IndexBuffer.IsValid())
			{
				Renderer::Instance->DrawIndexed(draw.Mesh);
			}
			else
			{
				Renderer::Instance->Draw(draw.Mesh);
			}
		})
		.Iterate();
	}
}
