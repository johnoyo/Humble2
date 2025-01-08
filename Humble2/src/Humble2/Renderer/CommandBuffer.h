#pragma once

#include "RenderPassRenderer.h"

namespace HBL2
{
	class CommandBuffer
	{
	public:
		virtual RenderPassRenderer* BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer) = 0;
		virtual void EndRenderPass(const RenderPassRenderer& renderPassRenderer) = 0;
		virtual void Submit() = 0;
	};
}

/*

class Renderer
{
	void Initialize();
	void ShutDown();

	void BeginFrame();
	CommandBuffer* BeginCommandRecording(CommandBufferType type);
	void EndFrame();
	void Present();
}


CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(COMMAND_BUFFER_TYPE::OFFSCREEN);


CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(COMMAND_BUFFER_TYPE::MAIN);
RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_renderPass, m_framebuffer); // m_framebuffer and m_renderPass is local to each render pass

// collect draws

passRenderer->drawSubpass(drawArea, draws);
commandBuffer->endRenderPass(passRenderer);
commandBuffer->submit();

CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(COMMAND_BUFFER_TYPE::UI);

---

class VulkanCommandBuffer : CommandBuffer
{
public:
	RenderPassRenderer* BeginRenderPass(Handle<RenderPass> renderPass, Handle<FrameBuffer> frameBuffer);
	virtual void EndRenderPass(const RenderPassRenderer& renderPassRenderer) override;
	virtual void Submit() override;

private:
	VkCommandPool m_CommandPool;
	VkCommandBuffer m_CommandBuffer;
	Handle<RenderPass> m_RenderPass;
	Handle<FrameBuffer> m_FrameBuffer;
}

class VulkanRenderPassRenderer : RenderPassRenderer
{
public:
	virtual void DrawSubPass() override;

private:
	VkCommandBuffer m_CommandBuffer;
}

*/

/*

while()
{
	BeginFrame();

	Renderer::Instance->BeginFrame();
	m_Specification.Context->OnUpdate(m_DeltaTime);
	Renderer::Instance->EndFrame();

	ImGuiRenderer::Instance->BeginFrame();
	m_Specification.Context->OnGuiRender(m_DeltaTime);
	ImGuiRenderer::Instance->EndFrame();

	EndFrame();
}
	
*/

/*

class RenderPass
{
	void Bake();
}

void OnUpdate()
{
	auto& renderables = m_Context->GetRegistry().group<Component::StaticMesh_New>(entt::get<Component::Transform>)

	RenderPass::Shadow->Bake();
	RenderPass::Main->Bake();
	RenderPass::UI->Bake();
}

*/