#include "SpriteRenderingSystem.h"

#include "Core/Window.h"

namespace HBL2
{
	void SpriteRenderingSystem::OnCreate()
	{
		auto* rm = ResourceManager::Instance;

		m_EditorScene = rm->GetScene(Context::EditorScene);
		m_UniformRingBuffer = Renderer::Instance->TempUniformRingBuffer;

		float* vertexBuffer = new float[30] {
			-0.5, -0.5, 0.0, 0.0, 1.0, // 0 - Bottom left
			 0.5, -0.5, 0.0, 1.0, 1.0, // 1 - Bottom right
			 0.5,  0.5, 0.0, 1.0, 0.0, // 2 - Top right
			 0.5,  0.5, 0.0, 1.0, 0.0, // 2 - Top right
			-0.5,  0.5, 0.0, 0.0, 0.0, // 3 - Top left
			-0.5, -0.5, 0.0, 0.0, 1.0, // 0 - Bottom left
		};

		m_VertexBuffer = rm->CreateBuffer({
			.debugName = "quad_vertex_buffer",
			.usage = BufferUsage::VERTEX,
			.byteSize = sizeof(float) * 30,
			.initialData = vertexBuffer,
		});

		m_SpriteMesh = rm->CreateMesh({
			.debugName = "quad_mesh",
			.vertexOffset = 0,
			.vertexCount = 6,
			.vertexBuffers = { m_VertexBuffer },
		});

		m_Context->GetRegistry()
			.group<Component::Sprite>(entt::get<Component::Transform>)
			.each([&](Component::Sprite& sprite, Component::Transform& transform)
			{
				if (sprite.Enabled)
				{
					HBL2_CORE_INFO("Setting up sprite");
				}
			});

		Handle<RenderPassLayout> renderPassLayout = rm->CreateRenderPassLayout({
			.debugName = "sprite-renderpass-layout",
			.depthTargetFormat = Format::D32_FLOAT,
			.subPasses = {
				{ .depthTarget = true, .colorTargets = 1, },
			},
		});

		m_RenderPass = rm->CreateRenderPass({
			.debugName = "sprite-renderpass",
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
					.nextUsage = TextureLayout::SHADER_READ_ONLY,
				},
			},
		});

		m_FrameBuffer = rm->CreateFrameBuffer({
			.debugName = "viewport",
			.width = Window::Instance->GetExtents().x,
			.height = Window::Instance->GetExtents().y,
			.renderPass = m_RenderPass,
			.depthTarget = Renderer::Instance->MainDepthTexture,
			.colorTargets = { Renderer::Instance->MainColorTexture },
		});

		Renderer::Instance->AddCallbackOnResize("Sprite-Resize-FrameBuffer", [this](uint32_t w, uint32_t h) { OnResize(w, h); });
	}

	void SpriteRenderingSystem::OnUpdate(float ts)
	{
		const glm::mat4& vp = GetViewProjection();

		Handle<BindGroup> globalBindings = Renderer::Instance->GetGlobalBindings2D();

		CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::MAIN, RenderPassStage::OpaqueSprite);
		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_RenderPass, m_FrameBuffer);

		ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&vp);

		DrawList draws;

		m_Context->GetRegistry()
			.group<Component::Sprite>(entt::get<Component::Transform>)
			.each([&](Component::Sprite& sprite, Component::Transform& transform)
			{
				if (sprite.Enabled)
				{
					if (!sprite.Material.IsValid())
					{
						return;
					}

					Material* material = ResourceManager::Instance->GetMaterial(sprite.Material);

					auto alloc = m_UniformRingBuffer->BumpAllocate<PerDrawDataSprite>();
					alloc.Data->Model = transform.WorldMatrix;
					alloc.Data->Color = material->AlbedoColor;

					draws.Insert({
						.Shader = material->Shader,
						.BindGroup = material->BindGroup,
						.Mesh = m_SpriteMesh,
						.Material = sprite.Material,
						.Offset = alloc.Offset,
						.Size = sizeof(PerDrawDataSprite),
					});
				}
			});

		GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings };
		passRenderer->DrawSubPass(globalDrawStream, draws);
		commandBuffer->EndRenderPass(*passRenderer);
		commandBuffer->Submit();

		/*
		
		DrawList& draws = GetDrawList3D();

		auto& renderPasses = Renderer::Instance->GetRenderPassess3D();

		renderPasses.Execute(RenderPassEvent::BeforeRendering);

		renderPasses.Execute(RenderPassEvent::BeforeRenderingShadows);
		ShadowPass();
		renderPasses.Execute(RenderPassEvent::AfterRenderingShadows);

		renderPasses.Execute(RenderPassEvent::BeforeRenderingOpaques);
		OpaqueGeometryPass();
		renderPasses.Execute(RenderPassEvent::AfterRenderingOpaques);

		renderPasses.Execute(RenderPassEvent::BeforeRenderingSkybox);
		SkyboxPass();
		renderPasses.Execute(RenderPassEvent::AfterRenderingSkybox);

		renderPasses.Execute(RenderPassEvent::BeforeRenderingTransparents);
		TransparentGeometryPass();
		renderPasses.Execute(RenderPassEvent::AfterRenderingTransparents);

		renderPasses.Execute(RenderPassEvent::BeforeRenderingPostProcess);
		PostProcessPass();
		renderPasses.Execute(RenderPassEvent::AfterRenderingPostProcess);

		renderPasses.Execute(RenderPassEvent::AfterRendering);	

		class RenderPassPool
		{
		public:
			void AddRenderPass(ScriptableRenderPass* renderPass);
			void SetUp(RenderPassEvent event);
			void Execute(RenderPassEvent event)
			{
				for (const auto& renderPass : renderPasses)
				{
					if (renderPass->GetInjectionPoint() == event)
					{
						renderPass->Excecute();
					}
				}
			}
			void CleanUp(RenderPassEvent event);

		private:
			std::vector<ScriptableRenderPass*> m_RenderPasses;
		};

		class ScriptableRenderPass
		{
		public:
			virtual void SetUp() = 0;
			virtual void Execute() = 0;
			virtual void CleanUp() = 0;

			const RenderPassEvent& GetInjectionPoint() const { return m_InjectionPoint; }

		protected:
			RenderPassEvent m_InjectionPoint;
			const char* m_PassName;
		};

		class DitherRenderPass : public ScriptableRenderPass
		{
		public:
			virtual void SetUp() override
			{
				m_PassName = "DitherRenderPass";
				m_InjectionPoint = RenderPassEvent::AfterRenderingPostProcess;
			}

			virtual void Execute() override
			{
				CommandBuffer* commandBuffer = Renderer::Instance->BeginCommandRecording(CommandBufferType::CUSTOM, RenderPassStage::PostProcess);
				RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(Renderer::Instance->GetMainRenderPass(), Renderer::Instance->GetMainFrameBuffer());

				GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings };
				passRenderer->DrawSubPass(globalDrawStream, draws);
				commandBuffer->EndRenderPass(*passRenderer);
				commandBuffer->Submit();
			}

			virtual void CleanUp() override
			{
			}
		};
		*/
	}

	void SpriteRenderingSystem::OnDestroy()
	{
		auto* rm = ResourceManager::Instance;

		rm->DeleteBuffer(m_VertexBuffer);
		rm->DeleteMesh(m_SpriteMesh);
		rm->DeleteRenderPass(m_RenderPass);
		rm->DeleteFrameBuffer(m_FrameBuffer);

		Renderer::Instance->RemoveOnResizeCallback("Sprite-Resize-FrameBuffer");
	}

	void SpriteRenderingSystem::OnResize(uint32_t width, uint32_t height)
	{
		ResourceManager::Instance->DeleteFrameBuffer(m_FrameBuffer);

		m_FrameBuffer = ResourceManager::Instance->CreateFrameBuffer({
			.debugName = "viewport",
			.width = width,
			.height = height,
			.renderPass = m_RenderPass,
			.depthTarget = Renderer::Instance->MainDepthTexture,
			.colorTargets = { Renderer::Instance->MainColorTexture },
		});
	}

	const glm::mat4& SpriteRenderingSystem::GetViewProjection()
	{
		if (Context::Mode == Mode::Runtime)
		{
			if (m_Context->MainCamera != entt::null)
			{
				return m_Context->GetComponent<Component::Camera>(m_Context->MainCamera).ViewProjectionMatrix;
			}
			else
			{
				HBL2_CORE_WARN("No main camera set for runtime context.");
			}
		}
		else if (Context::Mode == Mode::Editor)
		{
			if (m_EditorScene->MainCamera != entt::null)
			{
				return m_EditorScene->GetComponent<Component::Camera>(m_EditorScene->MainCamera).ViewProjectionMatrix;
			}
			else
			{
				HBL2_CORE_WARN("No main camera set for editor context.");
			}
		}
		else
		{
			HBL2_CORE_WARN("No mode set for current context.");
		}

		return glm::mat4(1.0f);
	}
}
