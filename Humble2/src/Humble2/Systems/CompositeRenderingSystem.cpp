#include "CompositeRenderingSystem.h"

#include "Utilities\ShaderUtilities.h"
#include "Utilities\TextureUtilities.h"

namespace HBL2
{
	void CompositeRenderingSystem::OnCreate()
	{
		auto* rm = ResourceManager::Instance;

		Handle<RenderPassLayout> renderPassLayout = rm->CreateRenderPassLayout({
			.debugName = "composite-renderpass-layout",
			.depthTargetFormat = Format::D32_FLOAT,
			.subPasses = {
				{ .depthTarget = true, .colorTargets = 1, },
			},
		});

		m_RenderPass = rm->CreateRenderPass({
			.debugName = "composite-renderpass",
			.layout = renderPassLayout,
			.depthTarget = {
				.loadOp = LoadOperation::LOAD,
				.storeOp = StoreOperation::STORE,
				.stencilLoadOp = LoadOperation::DONT_CARE,
				.stencilStoreOp = StoreOperation::DONT_CARE,
				.prevUsage = TextureLayout::DEPTH_STENCIL,
				.nextUsage = TextureLayout::DEPTH_STENCIL,
			},
			.colorTargets = {
				{
					.loadOp = LoadOperation::LOAD,
					.storeOp = StoreOperation::STORE,
					.prevUsage = TextureLayout::RENDER_ATTACHMENT,
					.nextUsage = TextureLayout::RENDER_ATTACHMENT,
				},
			},
		});

		float* vertexBuffer = nullptr;

		if (Renderer::Instance->GetAPI() == GraphicsAPI::VULKAN)
		{
			vertexBuffer = new float[24] {
				-1.0, -1.0, 0.0, 1.0, // 0 - Bottom left
				 1.0, -1.0, 1.0, 1.0, // 1 - Bottom right
				 1.0,  1.0, 1.0, 0.0, // 2 - Top right
				 1.0,  1.0, 1.0, 0.0, // 2 - Top right
				-1.0,  1.0, 0.0, 0.0, // 3 - Top left
				-1.0, -1.0, 0.0, 1.0, // 0 - Bottom left
			};
		}
		else if (Renderer::Instance->GetAPI() == GraphicsAPI::OPENGL)
		{
			vertexBuffer = new float[24] {
				-1.0, -1.0, 0.0, 0.0, // 0 - Bottom left
				 1.0, -1.0, 1.0, 0.0, // 1 - Bottom right
				 1.0,  1.0, 1.0, 1.0, // 2 - Top right
				 1.0,  1.0, 1.0, 1.0, // 2 - Top right
				-1.0,  1.0, 0.0, 1.0, // 3 - Top left
				-1.0, -1.0, 0.0, 0.0, // 0 - Bottom left
			};
		}

		m_VertexBuffer = rm->CreateBuffer({
			.debugName = "quad-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.byteSize = sizeof(float) * 24,
			.initialData = vertexBuffer,
		});

		m_QuadMesh = rm->CreateMesh({
			.debugName = "fullscreen-quad-mesh",
			.vertexOffset = 0,
			.vertexCount = 6,
			.vertexBuffers = { m_VertexBuffer },
		});

		m_QuadMaterial = rm->CreateMaterial({
			.debugName = "fullscreen-quad-material",
			.shader = ShaderUtilities::Get().GetBuiltInShader(BuiltInShader::PRESENT),
		});
	}

	void CompositeRenderingSystem::OnUpdate(float ts)
	{
		// Post Process Pass
		{
			/*CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::MAIN, RenderPassStage::PostProcess);
			RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_RenderPass, m_FrameBuffer);

			commandBuffer->EndRenderPass(*passRenderer);
			commandBuffer->Submit();*/
		}

		// Render final image to full screen quad
		{
			CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::MAIN, RenderPassStage::Present);
			RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(Renderer::Instance->GetMainRenderPass(), Renderer::Instance->GetMainFrameBuffer());

			DrawList draws;
			draws.Insert({
				.Shader = ShaderUtilities::Get().GetBuiltInShader(BuiltInShader::PRESENT),
				.Mesh = m_QuadMesh,
				.Material = m_QuadMaterial,
			});

			GlobalDrawStream globalDrawStream = { .BindGroup = Renderer::Instance->GetGlobalPresentBindings() };
			passRenderer->DrawSubPass(globalDrawStream, draws);
			commandBuffer->EndRenderPass(*passRenderer);
			commandBuffer->Submit();
		}
	}

	void CompositeRenderingSystem::OnDestroy()
	{
		auto* rm = ResourceManager::Instance;

		rm->DeleteBuffer(m_VertexBuffer);
		rm->DeleteMesh(m_QuadMesh);
		rm->DeleteMaterial(m_QuadMaterial);
		rm->DeleteRenderPass(m_RenderPass);
	}
}
