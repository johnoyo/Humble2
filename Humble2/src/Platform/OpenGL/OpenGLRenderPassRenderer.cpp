#include "OpenGLRenderPassRenderer.h"

namespace HBL2
{
	void OpenGLRenderPasRenderer::DrawSubPass(const GlobalDrawStream& globalDraw, DrawList& draws)
	{
		OpenGLResourceManager* rm = (OpenGLResourceManager*)ResourceManager::Instance;

		if (globalDraw.BindGroup.IsValid())
		{
			Renderer::Instance->WriteBuffer(globalDraw.BindGroup, 0);
			Renderer::Instance->WriteBuffer(globalDraw.BindGroup, 1);
		}

		for (auto&& [shaderID, drawList] : draws.m_Draws)
		{
			auto& localDraw = drawList[0];

			Renderer::Instance->WriteBuffer(localDraw.BindGroup, 0);
			Renderer::Instance->SetPipeline(localDraw.Shader);

			for (auto& draw : drawList)
			{
				Renderer::Instance->SetBuffers(draw.Mesh, draw.Material);
				Renderer::Instance->SetBindGroups(draw.Material);
				Renderer::Instance->SetBindGroup(draw.BindGroup, 0, draw.Offset, draw.Size);

				Mesh* openGLMesh = ResourceManager::Instance->GetMesh(draw.Mesh);

				if (openGLMesh->IndexBuffer.IsValid())
				{
					Renderer::Instance->DrawIndexed(draw.Mesh);
				}
				else
				{
					Renderer::Instance->Draw(draw.Mesh);
				}
			}
		}
	}
}
