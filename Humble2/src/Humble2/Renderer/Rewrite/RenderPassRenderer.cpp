#include "RenderPassRenderer.h"

#include "Renderer.h"

namespace HBL2
{
	void RenderPassRenderer::DrawSubPass(const GlobalDrawStream& globalDraw, const std::vector<LocalDrawStream>& draws)
	{
		if (globalDraw.BindGroup.IsValid())
		{
			// Bind global bind group
		}

		for (const auto& draw : draws)
		{
			//Renderer::Instance->SetPipeline(draw.Shader);
			//Renderer::Instance->SetBuffers(draw.Mesh);
			//Renderer::Instance->SetBindGroups(staticMesh.Material);

			//// TODO: Update uniforms
			//// ...
			//Material* openGLMaterial = ResourceManager::Instance->GetMaterial(staticMesh.Material);

			//glm::mat4 mvp = vp * transform.WorldMatrix;
			//Renderer::Instance->WriteBuffer(openGLMaterial->BindGroup, 0, &mvp);

			//glm::vec4 color = glm::vec4(1.0, 1.0, 0.75, 1.0);
			//Renderer::Instance->WriteBuffer(openGLMaterial->BindGroup, 1, &color);

			//Renderer::Instance->Draw(draw.Mesh);
		}
	}
}
