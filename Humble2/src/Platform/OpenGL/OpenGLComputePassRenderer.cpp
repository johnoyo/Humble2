#include "OpenGLComputePassRenderer.h"

#include "Resources\OpenGLShader.h"
#include "OpenGLResourceManager.h"

namespace HBL2
{
	void OpenGLComputePassRenderer::Dispatch(const Span<const HBL2::Dispatch>& dispatches)
	{
		OpenGLResourceManager* rm = (OpenGLResourceManager*)ResourceManager::Instance;

		for (const auto& dispatch : dispatches)
		{
			OpenGLShader* shader = rm->GetShader(dispatch.Shader);

			glUseProgram(shader->Program);
			glDispatchCompute(dispatch.ThreadGroupCount.x, dispatch.ThreadGroupCount.y, dispatch.ThreadGroupCount.z);
		}
	}
}