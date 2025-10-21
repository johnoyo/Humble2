#include "DebugRenderer.h"

#include "Core/Context.h"
#include "Core/Window.h"
#include "Physics/PhysicsEngine3D.h"
#include "Utilities/ShaderUtilities.h"

namespace HBL2
{
	DebugRenderer* DebugRenderer::Instance = nullptr;

	void DebugRenderer::Initialize()
	{
		m_ResourceManager = ResourceManager::Instance;

		m_DebugRenderPassLayout = m_ResourceManager->CreateRenderPassLayout({
			.debugName = "debug-renderpass-layout",
			.depthTargetFormat = Format::D32_FLOAT,
			.subPasses = {
				{ .depthTarget = true, .colorTargets = 1, },
			},
		});

		// Renderpass and framebuffer for debug drawing.
		m_DebugRenderPass = m_ResourceManager->CreateRenderPass({
			.debugName = "debug-renderpass",
			.layout = m_DebugRenderPassLayout,
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
					.format = Format::BGRA8_UNORM,
					.loadOp = LoadOperation::LOAD,
					.storeOp = StoreOperation::STORE,
					.prevUsage = TextureLayout::RENDER_ATTACHMENT,
					.nextUsage = TextureLayout::RENDER_ATTACHMENT,
				},
			},
		});

		m_DebugFrameBuffer = m_ResourceManager->CreateFrameBuffer({
			.debugName = "debug-viewport-fb",
			.width = Window::Instance->GetExtents().x,
			.height = Window::Instance->GetExtents().y,
			.renderPass = m_DebugRenderPass,
			.depthTarget = Renderer::Instance->MainDepthTexture,
			.colorTargets = { Renderer::Instance->MainColorTexture },
		});

		// Resize debug draw framebuffer callback.
		Renderer::Instance->AddCallbackOnResize("Resize-Debug-FrameBuffer", [this](uint32_t width, uint32_t height)
		{
			m_ResourceManager->DeleteFrameBuffer(m_DebugFrameBuffer);

			m_DebugFrameBuffer = m_ResourceManager->CreateFrameBuffer({
				.debugName = "debug-viewport-fb",
				.width = width,
				.height = height,
				.renderPass = m_DebugRenderPass,
				.depthTarget = Renderer::Instance->MainDepthTexture,
				.colorTargets = { Renderer::Instance->MainColorTexture },
			});
		});

		// Line vertex buffer.
		m_LineVertexBuffer = ResourceManager::Instance->CreateBuffer({
			.debugName = "debug-line-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.usageHint = BufferUsageHint::DYNAMIC,
			.memoryUsage = MemoryUsage::CPU_GPU,
			.byteSize = s_MaxDebugVertices * sizeof(DebugVertex), // TODO: Fix with max allowed vertex number.
			.initialData = nullptr,
		});

		// Reserve some space for the line vertices to avoid multiple allocations.
		m_LineVerts.resize(s_MaxDebugVertices);

		// Shader variant descriptions.
		ShaderDescriptor::RenderPipeline::Variant lineVariant = {};
		lineVariant.topology = Topology::LINE_LIST;
		lineVariant.blend.enabled = false;
		lineVariant.depthTest.enabled = false;
		lineVariant.depthTest.writeEnabled = false;
		lineVariant.depthTest.stencilEnabled = false;
		lineVariant.frontFace = FrontFace::CLOCKWISE;
		lineVariant.shaderHashKey = Random::UInt64(); // Create a random UUID since we do not have an asset to retrieve from there the UUID.

		// Compile debug shader.
		const auto& debugShaderCode = ShaderUtilities::Get().Compile("assets/shaders/debug-draw.shader");

		// Create debug shader handle.
		m_DebugShader = m_ResourceManager->CreateShader({
			.debugName = "debug-draw-shader",
			.VS { .code = debugShaderCode[0], .entryPoint = "main" },
			.FS { .code = debugShaderCode[1], .entryPoint = "main" },
			.bindGroups {
				Renderer::Instance->GetDebugBindingsLayout(),	// Global bind group (0)
			},
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = 16,
						.attributes = {
							{ .byteOffset = 0, .format = VertexFormat::FLOAT32x3 },
							{ .byteOffset = 12, .format = VertexFormat::UINT32 },
						},
					}
				},
				.variants = { lineVariant },
			},
			.renderPass = m_DebugRenderPass,
		});

		// Debug line material.
		m_DebugLineMaterial = m_ResourceManager->CreateMaterial({
			.debugName = "debug-material",
			.shader = m_DebugShader,
		});

		Material* lineMat = m_ResourceManager->GetMaterial(m_DebugLineMaterial);
		lineMat->VariantDescriptor = lineVariant;
		m_DebugLineMaterialVariantHash = ResourceManager::Instance->GetShaderVariantHash(lineMat->VariantDescriptor);	
	}
	void DebugRenderer::BeginFrame()
	{
		m_Draws.Reset();
		m_CurrentLineIndex = 0;

		if (PhysicsEngine3D::Instance != nullptr)
		{
			PhysicsEngine3D::Instance->OnDebugDraw();	
		}

		Color = Color::Red;
		DrawLine({ 0,0,0 }, { 1,0,0 });    // +X
		Color = Color::Green;
		DrawLine({ 0,0,0 }, { 0,1,0 });    // +Y
		Color = Color::Blue;
		DrawLine({ 0,0,0 }, { 0,0,1 });    // +Z
	}

	void DebugRenderer::EndFrame()
	{
		if (m_CurrentLineIndex != 0)
		{
			// Update dynamic line vertex buffer.
			m_ResourceManager->SetBufferData(m_LineVertexBuffer, 0, m_LineVerts.data());
			m_ResourceManager->MapBufferData(m_LineVertexBuffer, 0, m_CurrentLineIndex * sizeof(DebugVertex));

			// Gather line draws.
			m_Draws.Insert({
				.Shader = m_DebugShader,
				.Material = m_DebugLineMaterial,
				.VariantHash = m_DebugLineMaterialVariantHash,
				.VertexBuffer = m_LineVertexBuffer,
				.VertexCount = (uint32_t)m_CurrentLineIndex,
			});
		}
	}

	void DebugRenderer::Flush(CommandBuffer* commandBuffer)
	{
		const auto& cameraMVP = GetCameraMVP();

		Handle<BindGroup> globalBindings = Renderer::Instance->GetDebugBindings();
		ResourceManager::Instance->SetBufferData(globalBindings, 0, (void*)&cameraMVP);

		RenderPassRenderer* passRenderer = commandBuffer->BeginRenderPass(m_DebugRenderPass, m_DebugFrameBuffer);

		GlobalDrawStream globalDrawStream = { .BindGroup = globalBindings };
		passRenderer->DrawSubPass(globalDrawStream, m_Draws);

		commandBuffer->EndRenderPass(*passRenderer);
	}

	void DebugRenderer::Clean()
	{
		m_ResourceManager->DeleteBuffer(m_LineVertexBuffer);

		m_ResourceManager->DeleteRenderPassLayout(m_DebugRenderPassLayout);
		m_ResourceManager->DeleteRenderPass(m_DebugRenderPass);
		m_ResourceManager->DeleteFrameBuffer(m_DebugFrameBuffer);

		m_ResourceManager->DeleteShader(m_DebugShader);
		m_ResourceManager->DeleteMaterial(m_DebugLineMaterial);

		m_LineVerts.clear();
	}

	void DebugRenderer::DrawLine(const glm::vec3& from, const glm::vec3& to)
	{
		m_LineVerts[m_CurrentLineIndex++] = { from, Color };
		m_LineVerts[m_CurrentLineIndex++] = { to, Color };
	}
	void DebugRenderer::DrawRay(const glm::vec3& from, const glm::vec3& direction)
	{
	}
	void DebugRenderer::DrawSphere(const glm::vec3& position, float radius)
	{
	}
	void DebugRenderer::DrawWireSphere(const glm::vec3& position, float radius)
	{
	}
	void DebugRenderer::DrawCube(const glm::vec3& center, const glm::vec3& size)
	{
	}
	void DebugRenderer::DrawWireCube(const glm::vec3& center, const glm::vec3& size)
	{
	}
	
	glm::mat4 DebugRenderer::GetCameraMVP()
	{
		Entity mainCamera = Entity::Null;
		Scene* scene = nullptr;

		if (Context::Mode == Mode::Runtime)
		{
			scene = m_ResourceManager->GetScene(Context::ActiveScene);

			if (scene->MainCamera != Entity::Null)
			{
				mainCamera = scene->MainCamera;
			}
			else
			{
				HBL2_CORE_WARN("No main camera set for runtime context.");
			}
		}
		else if (Context::Mode == Mode::Editor)
		{
			scene = m_ResourceManager->GetScene(Context::EditorScene);

			if (scene->MainCamera != Entity::Null)
			{
				mainCamera = scene->MainCamera;
			}
			else
			{
				HBL2_CORE_WARN("No main camera set for editor context.");
			}
		}

		if (scene == nullptr || mainCamera == Entity::Null)
		{
			return glm::mat4(1.0f);
		}

		Component::Camera& camera = scene->GetComponent<Component::Camera>(mainCamera);

		// TODO: Fix once and for all the viewport discrepencies between the APIs!
		switch (Renderer::Instance->GetAPI())
		{
		case GraphicsAPI::OPENGL:
			auto proj = camera.Projection;
			proj[1][1] *= -1.0f;
			return proj * camera.View;
		case GraphicsAPI::VULKAN:
			return camera.ViewProjectionMatrix;
		}
	}
}