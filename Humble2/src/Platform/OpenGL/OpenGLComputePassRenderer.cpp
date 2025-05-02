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
			OpenGLBindGroup* bindGroup = rm->GetBindGroup(dispatch.BindGroup);
			OpenGLBindGroupLayout* bindGroupLayout = rm->GetBindGroupLayout(bindGroup->BindGroupLayout);

			for (int i = 0; i < bindGroup->Textures.size(); i++)
			{
				OpenGLTexture* openGLTexture = rm->GetTexture(bindGroup->Textures[i]);
				openGLTexture->Bind(bindGroupLayout->TextureBindings[i].slot);
			}

			for (int i = 0; i < bindGroup->Buffers.size(); i++)
			{
				OpenGLBuffer* buffer = rm->GetBuffer(bindGroup->Buffers[i].buffer);

				glBindBufferBase(GL_UNIFORM_BUFFER, bindGroupLayout->BufferBindings[i].slot, buffer->RendererId);
			}

			glUseProgram(shader->Program);
			glDispatchCompute(dispatch.ThreadGroupCount.x, dispatch.ThreadGroupCount.y, dispatch.ThreadGroupCount.z);
		}
	}
}